#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#define BUF_SIZE 1024

static int connfd;
static bool stop = false;

void sigcbk( int sig )
{
    printf( "got sig ： %d \n", sig );

    if( sig == SIGTERM ){
        stop = true;
    }else if( sig == SIGURG )
    {
        
        // 由于信号可能会中断其他系统操作,因此可能会覆盖其他调用的错误码,所以要备份和恢复错误码
        int lerrno = errno;

        char buffer[ BUF_SIZE ];
        memset( buffer, '\0', BUF_SIZE );
        int ret = recv( connfd, buffer, BUF_SIZE-1, MSG_OOB );
        printf( "got %d bytes of oob data '%s'\n", ret, buffer );

        errno = lerrno;
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

    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    int sock = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sock >= 0 );

    int ret = bind( sock, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );

    ret = listen( sock, 5 );
    assert( ret != -1 );

    struct sockaddr_in client;
    socklen_t clen = sizeof( client );
    connfd = accept( sock, ( struct sockaddr* )&client, &clen );
    if ( connfd < 0 )
    {
        perror( "accept" );
    }
    else
    {
        struct sigaction sa;
        memset( &sa, '\0', sizeof( sa ) );
        sa.sa_handler = sigcbk;
        sa.sa_flags |= SA_RESTART;//中断后继续系统操作(会意外的中断系统调用)
        sigfillset( &sa.sa_mask );
        ret = sigaction( SIGURG, &sa, nullptr );
        if( ret == -1 )
        {
            perror( "sigaction SIGURG" );
        }
        ret = sigaction( SIGTERM, &sa, nullptr );
        if( ret == -1 )
        {
            perror( "sigaction SIGTERM" );
        }

        // 需要设置信号拥有者进程id
        fcntl( connfd, F_SETOWN, getpid() );

        while( !stop )
        {
            char buffer[ BUF_SIZE ];
            memset( buffer, '\0', BUF_SIZE );
            ret = recv( connfd, buffer, BUF_SIZE-1, 0 );
            if( ret > 0 )
            {
                printf( "got %d bytes of normal data '%s'\n", ret, buffer );
            }
            usleep(200000);
        }

        close( connfd );
    }

    close( sock );
    return 0;
}

