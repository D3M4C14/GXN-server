#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

static bool stop = false;
static int sigpfd[2];

// 设置描述符为非阻塞
int setnbk( int fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}

// 往epoll添加描述符
void addfd( int epollfd, int fd ,unsigned int ev = EPOLLIN | EPOLLET )
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev;
    epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
    setnbk( fd );
}

// 信号处理回调
void sigcbk( int sig )
{
    int lerrno = errno;
    send( sigpfd[1], (char*)&sig, 1, 0 );
    errno = lerrno;
}

// 添加信号监听
void addsig( int sig )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = sigcbk;
    sa.sa_flags |= SA_RESTART;
    sigfillset( &sa.sa_mask );
    int ret = sigaction( sig, &sa, nullptr );
    if( ret == -1 )
    {
        perror( "sigaction" );
    }
}

int main( int argc, char* argv[] )
{
    if( argc <= 2 )
    {
        printf( "arg: ip port\n" );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );

    printf( "* runing %s : %s:%d \n", basename( argv[0] ), ip, port );

    int ret = 0;
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );

    ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );

    ret = listen( listenfd, 5 );
    assert( ret != -1 );

    int epollfd = epoll_create( 20 );
    if( epollfd == -1 )
    {
        perror( "epoll_create" );
    }

    addfd( epollfd, listenfd );

    ret = socketpair( PF_UNIX, SOCK_STREAM, 0, sigpfd );
    if( ret == -1 ){
        perror( "socketpair" );
    }
    // 信号写端管道非阻塞
    setnbk( sigpfd[1] );

    // 信号读端加入epoll
    addfd( epollfd, sigpfd[0] );

    // 添加信号
    addsig( SIGHUP );
    addsig( SIGCHLD );
    addsig( SIGTERM );
    addsig( SIGINT );

    const int evnum = 64;
    epoll_event events[ evnum ];

    while( !stop )
    {
        int n = epoll_wait( epollfd, events, evnum, -1 );
        if ( ( n < 0 ) && ( errno != EINTR ) )
        {
            perror( "epoll_wait" );
            break;
        }
    
        for ( int i = 0; i < n; i++ )
        {
            int fd = events[i].data.fd;
            if( fd == listenfd )
            {// 有新的连接
                struct sockaddr_in client;
                socklen_t clen = sizeof( client );
                int connfd = accept( fd, ( struct sockaddr* )&client, &clen );
                // 客户端链接信息
                char remote[INET_ADDRSTRLEN];
                printf( "client connected : %s:%d\n", 
                            inet_ntop( AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN ), ntohs( client.sin_port ) );

                addfd( epollfd, connfd, EPOLLIN | EPOLLET | EPOLLRDNORM | EPOLLPRI);
            }
            else if( ( fd == sigpfd[0] ) && ( events[i].events & EPOLLIN ) )
            {// 信号
                char signals[32];
                ret = recv( fd, signals, sizeof( signals ), 0 );
                if( ret < 1 )
                {
                    continue;
                }
                else
                {
                    for( int i = 0; i < ret; ++i )
                    {
                        printf( "got sig : %d\n", signals[i] );
                        switch( signals[i] )
                        {
                            case SIGTERM:
                            case SIGINT:
                            {
                                stop = true;
                            }
                            case SIGCHLD:
                            case SIGHUP:
                            ;
                        }
                    }
                }
            }
            else
            {// 客户端数据
                const int buflen = 1024;
                char buf[buflen];

                if( events[i].events & EPOLLPRI )
                {
                    memset( buf, '\0', buflen );
                    ret = recv( fd, buf, buflen-1, MSG_OOB );
                    if( ret <= 0 )
                    {
                        if( errno != EAGAIN && errno != EWOULDBLOCK )
                        {
                            perror( "recv oob" );
                        }
                    }
                    else
                    {
                        printf( "get %d bytes of oob data: %s\n", ret, buf );
                    }
                }

                if( events[i].events & EPOLLRDNORM )
                {
                    while( true )
                    {
                        memset( buf, '\0', buflen );
                        ret = recv( fd, buf, buflen-1, 0 );
                        if( ret <= 0 )
                        {
                            if( errno == EAGAIN || errno == EWOULDBLOCK )
                            {
                                break;
                            }
                            else
                            {
                                perror( "recv" );
                                break;
                            }
                        }
                        else
                        {
                            printf( "get %d bytes of normal data: %s\n", ret, buf );
                        }
                    }
                }
            }
        }
    }

    printf( "close fds\n" );
    close( listenfd );
    close( epollfd );
    close( sigpfd[1] );
    close( sigpfd[0] );
    return 0;
}
