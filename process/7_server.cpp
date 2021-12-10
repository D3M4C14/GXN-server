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
    char buf[0];
    struct iovec iov[1];
    iov[0].iov_base = buf;
    iov[0].iov_len = 1;

    cmsghdr cm;
    cm.cmsg_len = CONTROL_LEN;
    cm.cmsg_level = SOL_SOCKET;
    cm.cmsg_type = SCM_RIGHTS; //代表文件描述符 还可以是凭证
    *(int *)CMSG_DATA( &cm ) = sfd;

    struct msghdr msg;
    msg.msg_name    = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov     = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    sendmsg( fd, &msg, 0 );
}

int recv_fd( int fd )
{
    char buf[0];
    struct iovec iov[1];
    iov[0].iov_base = buf;
    iov[0].iov_len = 1;

    cmsghdr cm;

    struct msghdr msg;
    msg.msg_name    = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov     = iov;
    msg.msg_iovlen  = 1;
    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    recvmsg( fd, &msg, 0 );

    return *(int *)CMSG_DATA( &cm );
}

int main( int argc, char* argv[] )
{
    int pfd[2];

    int ret = socketpair( PF_UNIX, SOCK_DGRAM, 0, pfd );
    if( ret == -1 )
    {
        perror("socketpair");
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
        int fd = open( "test.txt", O_CREAT | O_RDWR | O_TRUNC, 0666 );
        if ( fd < 0 )
        {
            perror( "open test.txt" );
            return 1;
        }

        syn( fd, "abc123", 6 );
        
        printf( "process write data\n" );

        send_fd( pfd[1], fd );

        close( fd );
        exit( 0 );

    }
    else
    {
        printf( "pppp\n" );
        sleep(1);
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
