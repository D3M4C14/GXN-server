#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int main( int argc, char* argv[] )
{
    if( argc <= 3 )
    {
        printf( "arg: ip port backlog\n" );
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi( argv[2] );
    int backlog = atoi( argv[3] );
    
    printf( "* runing %s : %s:%d backlog:%d \n", basename( argv[0] ), ip, port, backlog );

    int sock = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sock >= 0 );

    // 地址重用 端口状态位于 TIME_WAIT 可以重用端口
    int one = 1;
    setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) );

    // 设置在线接收带外数据
    int on = 1;
    setsockopt( sock, SOL_SOCKET, SO_OOBINLINE, &on, sizeof(on) );

    // 设置写缓冲区大小
    int sbsz = 4096;
    int slen = sizeof( sbsz );
    setsockopt( sock, SOL_SOCKET, SO_SNDBUF, &sbsz, sizeof( sbsz ) );
    getsockopt( sock, SOL_SOCKET, SO_SNDBUF, &sbsz, ( socklen_t* )&slen );
    printf( "* the send buffer size after setting is %d\n", sbsz );

    // 设置读缓冲区大小
    int rbsz = 4096;
    int rlen = sizeof( rbsz );
    setsockopt( sock, SOL_SOCKET, SO_RCVBUF, &rbsz, sizeof( rbsz ) );
    getsockopt( sock, SOL_SOCKET, SO_RCVBUF, &rbsz, ( socklen_t* )&rlen );
    printf( "* the receive buffer size after settting is %d\n", rbsz );

    // 设置地址和端口
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    // 绑定
    int ret = bind( sock, (struct sockaddr*)&address, sizeof( address ) );
    assert( ret != -1 );

    // 监听
    ret = listen( sock, backlog );
    assert( ret != -1 );
    
    struct sockaddr_in client;
    socklen_t clen = sizeof( client );
    int connfd = accept( sock, (struct sockaddr*)&client, &clen );
    if ( connfd < 0 )
    {
        perror( "accept" );
    }
    else
    {
        // 客户端链接信息
        char remote[INET_ADDRSTRLEN];
        printf( "client connected : %s:%d\n", 
                    inet_ntop( AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN ), ntohs( client.sin_port ) );

        const int bsz = 4096;
        char buffer[ bsz ];

        // 收发数据
        while ( true )
        {
            usleep( 100000 );

            bool oob = false;
            ret = sockatmark( connfd );
            if( ret == 1 )
            {
                oob = true;
                printf( "at OOB mark\n" );
            }
            else if( ret == -1 )
            {
                perror( "sockatmark" );
            }

            memset( buffer, '\0', bsz );
            ret = recv( connfd, buffer, bsz-1, 0 );
            printf( "from client got %d bytes of data: '%s'\n", ret, buffer );

            sprintf( buffer, "got msg size:%d", ret );
            send( connfd, buffer, strlen( buffer ), 0 );

            if( oob )
            {
                sprintf( buffer, "<got oob msg>");
                send( connfd, buffer, strlen( buffer ), MSG_OOB );
            }
            
        }

        close( connfd );
    }

    close( sock );
    return 0;
}
