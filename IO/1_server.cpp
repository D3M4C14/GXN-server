#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>


int main( int argc, char* argv[] )
{
    if( argc <= 3 )
    {
        printf( "arg: ip port file\n" );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );

    int filefd = open( argv[3], O_CREAT | O_WRONLY | O_TRUNC, 0666 );
    assert( filefd > 0 );

    printf( "* runing %s : %s:%d file:%s \n", basename( argv[0] ), ip, port, argv[3] );
    printf( "* print log write to file:%s \n", argv[3] );

    close( STDOUT_FILENO );
    dup( filefd );

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
    int connfd = accept( sock, ( struct sockaddr* )&client, &clen );
    if ( connfd < 0 )
    {
        printf( "errno is: %d\n", errno );
    }
    else
    {
        // 客户端链接信息
        char remote[INET_ADDRSTRLEN];
        printf( "client connected : %s:%d\n", 
                    inet_ntop( AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN ), ntohs( client.sin_port ) );

        int sfd[2];
        int ret = pipe( sfd );
        assert( ret != -1 );

        int ffd[2];
        ret = pipe( ffd );
        assert( ret != -1 );

        const int sz = 32768;

        while(true)
        {
            
            usleep(100000);

            ret = splice( connfd, nullptr, sfd[1], nullptr, sz, SPLICE_F_MORE | SPLICE_F_MOVE );
            if(ret == -1)
            {
                perror( "splice1" );
            }
            
            write( connfd, "recvmsg\n", 8 );

            // 不可以直接转
            // ret = tee( sfd[0], connfd, sz, SPLICE_F_NONBLOCK );
            // if(ret == -1)
            // {
            //     perror( "tee" );
            // }

            // 只能管道间转
            ret = tee( sfd[0], ffd[1], sz, SPLICE_F_NONBLOCK );
            if(ret == -1)
            {
                perror( "tee" );
            }

            ret = splice( sfd[0], nullptr, connfd, nullptr, sz, SPLICE_F_MORE | SPLICE_F_MOVE );
            if(ret == -1)
            {
                perror( "splice2" );
            }

            ret = splice( ffd[0], nullptr, filefd, nullptr, sz, SPLICE_F_MORE | SPLICE_F_MOVE );
            if(ret == -1)
            {
                perror( "splice3" );
            }

        }

        close( connfd );
        close( sfd[0] );
        close( sfd[1] );
        close( ffd[0] );
        close( ffd[1] );
    }
    

    close( sock );
    return 0;
}

