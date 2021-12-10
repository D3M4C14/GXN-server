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
#include <poll.h>

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

    pollfd fds[1];

    fds[0].fd = connfd;
    fds[0].events = POLLIN | POLLPRI | POLLRDNORM;
    fds[0].revents = 0;

    while( true )
    {
        // -1 无限阻塞 
        ret = poll( fds, 1, -1 );

        if ( ret < 0 )
        {
            perror("poll");
            break;
        }
        
        if( fds[0].revents & POLLPRI )
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

        if( fds[0].revents & POLLRDNORM )
        {
            memset( buf, '\0', buflen );
            ret = recv( connfd, buf, buflen-1, 0 );
            if( ret < 0 )
            {
                perror( "recv" );
                break;
            }
            else if( ret > 0 )
            {
                printf( "get %d bytes of normal data: %s\n", ret, buf );    
            }
        }

    }

    close( connfd );
    close( listenfd );
    return 0;
}
