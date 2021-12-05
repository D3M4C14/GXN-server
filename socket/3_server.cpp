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
    if( argc <= 2 )
    {
        printf( "arg: ip port\n" );
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi( argv[2] );
    
    printf( "* runing %s : %s:%d \n", basename( argv[0] ), ip, port );

    // socket
    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );

    // 地址重用 端口状态位于 TIME_WAIT 可以重用端口
    int one = 1;
    setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) );

    // 设置地址和端口
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    // 绑定
    int ret = bind( listenfd, (struct sockaddr*)&address, sizeof( address ) );
    assert( ret != -1 );

    // 监听
    ret = listen( listenfd, 5 );
    assert( ret != -1 );
    
    // UDP 设置地址和端口
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );
    int udpfd = socket( PF_INET, SOCK_DGRAM, 0 );
    assert( udpfd >= 0 );
    
    // UDP 绑定
    ret = bind( udpfd, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );

    // UDP client
    struct sockaddr_in udpclient;
    socklen_t uclen = sizeof( udpclient );

    // TCP client
    struct sockaddr_in tcpclient;
    socklen_t tclen = sizeof( tcpclient );

    // TCP 连接
    int tcpfd = accept( listenfd, (struct sockaddr*)&tcpclient, &tclen );
    if ( tcpfd < 0 )
    {
        perror("accept");
    }
    else
    {
        // 客户端链接信息
        char remote[INET_ADDRSTRLEN];
        printf( "tcp client connected : %s:%d\n", 
                    inet_ntop( AF_INET, &tcpclient.sin_addr, remote, INET_ADDRSTRLEN ), ntohs( tcpclient.sin_port ) );

        const int bsz = 4096;
        char buffer[ bsz ];
        memset( buffer, '\0', bsz );

        // 收发数据
        while ( true )
        {
            usleep(100000);

            // TCP 
            ret = recv( tcpfd, buffer, bsz-1, MSG_DONTWAIT );
            if( ret > 0 ){
                printf( "from client tcp got %d bytes of data: '%s'\n", ret, buffer );

                sprintf(buffer,"got msg size:%d",ret);
                send( tcpfd, buffer, strlen( buffer ), 0 );

                memset( buffer, '\0', bsz );
            }

            // UDP
            ret = recvfrom(udpfd, buffer, bsz-1, MSG_DONTWAIT, (struct sockaddr*)&udpclient, &uclen);
            if( ret > 0 ){
                printf( "from client udp got %d bytes of data: '%s'\n", ret, buffer );

                sprintf(buffer,"got msg size:%d",ret);
                send( udpfd, buffer, strlen( buffer ), 0 );

                memset( buffer, '\0', bsz );
            }
            
        }

        close( tcpfd );
        close( udpfd );
    }

    close( listenfd );
    return 0;
}
