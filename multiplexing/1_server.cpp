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

    const int buflen = 4;
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

    // 设置为非阻塞
    int old_option = fcntl( connfd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( connfd, F_SETFL, new_option );

    while( true )
    {
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
            perror( "select" );
            break;
        }
        else if( ret == 0 )
        {
            printf( "nothing happen\n" );
            continue;
        }

        printf( "select event time:%ld:%ld\n", tv.tv_sec, tv.tv_usec );
    
        // 优先检查带外数据 (如果只有一个带外数据开头,可能会被普通数据的读取而导致丢失)
        if( FD_ISSET( connfd, &efds ) )
        {
            memset( buf, '\0', buflen );
            ret = recv( connfd, buf, buflen-1, MSG_OOB );
            if( ret < 0 )
            {
                perror( "recv oob" );
                break;
            }
            else if( ret > 0 )
            {
                printf( "get %d bytes of oob data: %s\n", ret, buf );
            }
        }

        if ( FD_ISSET( connfd, &rfds ) )
        {
            memset( buf, '\0', buflen );
            ret = recv( connfd, buf, buflen-1, 0 );
            if( ret < 0 )
            {
                perror( "recv1" );
                break;
            }
            else if( ret > 0 )
            {
                printf( "get %d bytes of normal data: %s\n", ret, buf );
            }
        }

        if( FD_ISSET( connfd, &wfds ) )
        {
            printf( "can write data \n" );
            sleep(1);
        }

    }

    close( connfd );
    close( listenfd );
    return 0;
}
