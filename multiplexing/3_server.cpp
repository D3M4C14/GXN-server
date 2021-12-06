#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>

int main( int argc, char* argv[] )
{
    if( argc <= 2 )
    {
        printf( "arg: ip port\n" );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );

    printf( "* runing %s : %s:%d \n"
                        , basename( argv[0] ), ip, port );

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

    struct sockaddr_in caddr;
    socklen_t clen = sizeof( caddr );
    int connfd = accept( listenfd, ( struct sockaddr* )&caddr, &clen );
    if ( connfd < 0 )
    {
        printf( "errno is: %d\n", errno );
        close( listenfd );
    }

    char raddr[INET_ADDRSTRLEN];
    printf( "connected with ip: %s and port: %d\n"
                , inet_ntop( AF_INET, &caddr.sin_addr, raddr, INET_ADDRSTRLEN )
                , ntohs( caddr.sin_port ) );

    const int buflen = 1024;
    char buf[buflen];

    // 设置连接fd为非阻塞
    int old_option = fcntl( connfd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( connfd, F_SETFL, new_option );


    int epollfd = epoll_create( 1 );
    
    // 默认是LT模式,可以注册EPOLLET事件开启ET高效模式
    // 事件类型和poll基本一致,只是开头需要加一个字母E
    epoll_event event;
    event.data.fd = connfd;
    event.events = EPOLLIN | EPOLLPRI | EPOLLRDNORM | EPOLLET;
    epoll_ctl( epollfd, EPOLL_CTL_ADD, connfd, &event );

    epoll_event events[ 1 ];

    while( true )
    {
        memset( buf, '\0', buflen );

        // -1 无限阻塞 
        int en = epoll_wait( epollfd, events, 1, -1 );
        if ( ( en < 0 ) && ( errno != EINTR ) )
        {
            perror( "epoll" );
            break;
        }

        for ( int i = 0; i < en; i++ )
        {
            int fd = events[i].data.fd;
            if( fd == connfd )
            {
                if( events[i].events & EPOLLRDNORM )
                {
                    while( true )
                    {
                        ret = recv( fd, buf, buflen-1, 0 );
                        if( ret <= 0 )
                        {
                            if( ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) )
                            {
                                break;
                            }
                            else
                            {
                                perror("recv");
                            }
                        }
                        else
                        {
                            printf( "get %d bytes of normal data: %s\n", ret, buf );
                        }
                    }
                }
                else if( events[i].events & EPOLLPRI )
                {
                    while( true )
                    {
                        ret = recv( fd, buf, buflen-1, MSG_OOB );
                        if( ret <= 0 )
                        {
                            if( ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) )
                            {
                                break;
                            }
                            else
                            {
                                perror("recv oob");
                            }
                        }
                        else
                        {
                            printf( "get %d bytes of oob data: %s\n", ret, buf );
                        }
                    }
                }
                else
                {
                    printf( "other event: %d\n",events[i].events );
                }
            }
        }
    }

    close( connfd );
    close( listenfd );
    close( epollfd );
    return 0;
}
