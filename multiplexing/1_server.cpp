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

    fd_set rfds;//读
    fd_set wfds;//写
    fd_set efds;//异常(接收带外数据)

    FD_ZERO( &rfds );
    FD_ZERO( &wfds );
    FD_ZERO( &efds );

    // 若开启 则会放到普通数据中
    // int on = 1;
    // setsockopt( connfd, SOL_SOCKET, SO_OOBINLINE, &on, sizeof( on ) );

    while( true )
    {
        memset( buf, '\0', buflen );

        // 每次都要重设一次
        FD_SET( connfd, &rfds );
        FD_SET( connfd, &wfds );
        FD_SET( connfd, &efds );

        // 设置超时
        struct timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 0;

        // 阻塞 通常设置为最大fd+1
        ret = select( connfd + 1, &rfds, &wfds, &efds, &tv );

        if ( ret < 0 )
        {
            perror("select");
            break;
        }
        else if( ret == 0 )
        {
            printf( "nothing happen\n" );
            continue;
        }

        printf( "select event time:%ld:%ld\n", tv.tv_sec, tv.tv_usec );
    
        if ( FD_ISSET( connfd, &rfds ) )
        {
            ret = recv( connfd, buf, buflen-1, 0 );
            if( ret <= 0 )
            {
                break;
            }
            printf( "get %d bytes of normal data: %s\n", ret, buf );
        }
        else if( FD_ISSET( connfd, &efds ) )
        {
            ret = recv( connfd, buf, buflen-1, MSG_OOB );
            if( ret <= 0 )
            {
                break;
            }
            printf( "get %d bytes of oob data: %s\n", ret, buf );
        }
        else if( FD_ISSET( connfd, &wfds ) )
        {
            printf( "can write data \n" );
            sleep(1);
        }

    }

    close( connfd );
    close( listenfd );
    return 0;
}
