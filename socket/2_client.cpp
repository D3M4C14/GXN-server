#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>


#define DATA_LEN 121

static char data[][DATA_LEN]={
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    "cccccccccccccccccccccccccccccc"
    "dddddddddddddddddddddddddddddd",

    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
    "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
    "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"
    "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDD",

    "123456789123456789123456789123"
    "123456789123456789123456789123"
    "123456789123456789123456789123"
    "123456789123456789123456789123",

    "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
    "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
    "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"
    "qqqqqqqqqqqqqqqqqqqqqqqqqqqqqq",

    "zxcvbnmzxcvbnmzxcvbnmzxcvbnzxc"
    "zxcvbnmzxcvbnmzxcvbnmzxcvbnzxc"
    "zxcvbnmzxcvbnmzxcvbnmzxcvbnzxc"
    "zxcvbnmzxcvbnmzxcvbnmzxcvbnzxc",
};

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

    int sock = socket( PF_INET, SOCK_DGRAM, 0 );
    assert( sock >= 0 );
    
    // 设置地址和端口
    struct sockaddr_in address;
    bzero( &address, sizeof(address) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    socklen_t alen = sizeof( address );

    const int bsz = 4096;
    char buffer[ bsz ];
    while(1){

        printf("-input msg...\n");
        std::cin >> buffer;
        printf("\n");

        int blen = strlen(buffer);

        if( blen > 2 && buffer[0] == '@' && buffer[1] > '0' && buffer[1] <= '5' )
        {// 通用型 

            //统一写 发送
            struct iovec iovt[2];
            iovt[0].iov_base = buffer;
            iovt[0].iov_len = blen;
            iovt[1].iov_base = data[buffer[1]-'0'-1];
            iovt[1].iov_len = DATA_LEN-1;

            struct msghdr msg;
            msg.msg_name = &address;
            msg.msg_namelen = alen;
            msg.msg_iov = iovt;
            msg.msg_iovlen = 2;
            msg.msg_control = nullptr;
            msg.msg_controllen = 0;

            int ret = sendmsg(sock,&msg,MSG_DONTWAIT);
            if(ret<0){
                perror("sendmsg");
            }

            for (int i = 0; i < 3; ++i)
            {
                usleep(100000);

                // 分散读
                struct iovec iovo[1];
                struct msghdr msg;

                iovo[0].iov_base = buffer;
                iovo[0].iov_len = bsz-1;
                msg.msg_name = &address;
                msg.msg_namelen = alen;
                msg.msg_iov = iovo;
                msg.msg_iovlen = 1;
                msg.msg_control = nullptr;
                msg.msg_controllen = 0;

                ret = recvmsg( sock, &msg, MSG_DONTWAIT );
                if( ret == EAGAIN )
                {// 暂无消息
                    usleep(200000);
                    continue;
                }
                else if( ret > 0 ){
                    printf("recvmsg msg : '%s' \n", buffer );
                    break;
                }
            }
        }
        else
        {

            // 非阻塞 发送
            sendto(sock, buffer, blen, MSG_DONTWAIT, (struct sockaddr*)&address, alen);

            for (int i = 0; i < 3; ++i)
            {
                usleep(100000);
                int ret = recvfrom(sock, buffer, bsz-1, MSG_DONTWAIT, (struct sockaddr*)&address, &alen);
                if( ret == EAGAIN )
                {// 暂无消息
                    usleep(200000);
                    continue;
                }
                else if( ret > 0 ){
                    printf("recvfrom msg : '%s' \n", buffer );
                    break;
                }
            }

        }

    }

    close(sock);

    return 0;
}
