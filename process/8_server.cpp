#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static const int CONTROL_LEN = CMSG_LEN( sizeof(int) );

void send_fd( int fd, int sfd )
{
    cmsghdr cm;
    cm.cmsg_len = CONTROL_LEN;
    cm.cmsg_level = SOL_SOCKET;
    cm.cmsg_type = SCM_RIGHTS; //代表文件描述符 还可以是凭证
    *(int *)CMSG_DATA( &cm ) = sfd;

    struct msghdr msg;
    msg.msg_name    = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov     = nullptr;
    msg.msg_iovlen = 0;
    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    sendmsg( fd, &msg, 0 );
}

int recv_fd( int fd )
{
    cmsghdr cm;

    struct msghdr msg;
    msg.msg_name    = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov     = nullptr;
    msg.msg_iovlen  = 0;
    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    recvmsg( fd, &msg, 0 );

    return *(int *)CMSG_DATA( &cm );
}

int main( int argc, char* argv[] )
{
    int pfd[2];

    // 准备文件
    int tfd = open( "test.txt", O_CREAT | O_RDWR | O_TRUNC, 0666 );
    write( tfd, "abc123", 6 );
    close( tfd );

    int ret = socketpair( PF_UNIX, SOCK_DGRAM, 0, pfd );
    if( ret == -1 )
    {
        perror( "socketpair" );
        return 1;
    }

    pid_t pid = fork();
    if( pid < 0 )
    {
        perror( "fork" );
        return 1;
    }

    if ( pid == 0 )
    {
        int fd = open( "test.txt", O_RDWR, 0666 );
        if ( fd < 0 )
        {
            perror( "open test.txt" );
            return 1;
        }

        send_fd( pfd[1], fd );

        close( fd );
        exit( 0 );

    }
    else
    {
        int fd = recv_fd( pfd[0] );

        const int bsz = 64;
        char buf[bsz];
        memset( buf, '\0', bsz );
        read( fd, buf, bsz );
        printf( "I got fd %d and data : '%s'\n", fd, buf );
    
        close( fd );
    }

    close( pfd[0] );
    close( pfd[1] );

}
