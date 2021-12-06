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

    // 非阻塞 UDP
    int sock = socket( PF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0 );
    assert( sock >= 0 );

    // 地址重用 端口状态位于 TIME_WAIT 可以重用端口
    int one = 1;
    setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one) );

    // 设置地址和端口
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    // 绑定
    int ret = bind( sock, (struct sockaddr*)&address, sizeof( address ) );
    assert( ret != -1 );
    
    struct sockaddr_in client;
    socklen_t clen = sizeof( client );

    const int bsz = 4096;
    char buffer[bsz];
    memset( buffer, '\0', bsz );

    while( true )
    {
        usleep(100000);

        int ret = recvfrom(sock, buffer, bsz-1, 0, (struct sockaddr*)&client, &clen);

        if( ret == EAGAIN )
        {// 暂无消息
            usleep(200000);
            continue;
        }
        else if( ret > 0 ){

            // 客户端链接信息
            char remote[INET_ADDRSTRLEN];
            printf( "client info : %s:%d\n", 
                        inet_ntop( AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN ), ntohs( client.sin_port ) );

            printf("receive msg : '%s' \n", buffer );

            sprintf(buffer,"got msg size:%d",ret);
            sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*)&client, clen);

            memset( buffer, '\0', bsz );
        }

    }

    close( sock );
    return 0;
}
