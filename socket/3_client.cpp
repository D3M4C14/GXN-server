#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

int main( int argc, char* argv[] )
{
    if( argc <= 2 )
    {
        printf( "arg: ip port \n" );
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi( argv[2] );
    
    printf( "* runing %s : %s:%d \n", basename(argv[0]), ip, port );

    int sock = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sock >= 0 );
    
    // 设置地址和端口
    struct sockaddr_in address;
    bzero( &address, sizeof(address) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    socklen_t alen = sizeof( address );

    // 连接
    if( connect( sock, (struct sockaddr*)&address, alen ) < 0){
        perror("connect");
        return 1;
    }
    
    const int bsz = 4096;
    char buffer[ bsz ];
    while(1){

        printf("-input msg...\n");
        std::cin >> buffer;
        printf("\n");

        int len = strlen( buffer );
        if(len>1 && buffer[0]=='U' )
        {
            sendto(sock, buffer, len, MSG_DONTWAIT, (struct sockaddr*)&address, alen);
            printf( "sendto msg: '%s'\n", buffer );
        }
        else
        {
            send( sock, buffer, len, 0);
            printf( "send msg: '%s'\n", buffer );
        }

        if(strcmp(buffer,"quit")==0)break;

        usleep(100000);

        for (int i = 0; i < 10; ++i)
        {

            memset( buffer, '\0', bsz );
            len = recv( sock, buffer, bsz, MSG_DONTWAIT );
            if(len>0){
                printf( "from server msg: '%s'\n", buffer );
            }
            else
            {
                break;
            }
            usleep(10000);
        }

    }

    close(sock);

    return 0;
}
