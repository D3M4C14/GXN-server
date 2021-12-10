#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>

static int pfd[2];
const int procnum = 2;
static int exit_proc_num = 0;
static bool main_proc_stop = false;

// 信号处理回调
void sigcbk( int sig )
{
    int lerrno = errno;
    if( sig == SIGCHLD )
    {
        printf( "process(%d) got SIGCHLD sig\n", getpid() );

        pid_t pid;
        int stat;
        while( true )
        {
            pid = waitpid( -1, &stat, WNOHANG );
            if( pid < 0 )
            {
                perror( "waitpid" );
            }
            else if( pid > 0 )
            {
                printf( "sub process(%d) stat : %d \n", pid, stat );
                ++exit_proc_num;
                break;
            }
        }

        if( exit_proc_num >= procnum ) main_proc_stop = true;

    }
    
    errno = lerrno;
}

int main( int argc, char* argv[] )
{
    printf( " * run main process : %d \n", getpid() );

    pid_t pid;
    
    // 忽略一些危险信号(默认操作为结束进程)
    // 也可以在写端设置MSG_NOSIGNAL来避免发送这种信号
    signal( SIGPIPE, SIG_IGN );

    // 建立进程间通信的管道
    int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, pfd );
    if( ret == -1 )
    {
        perror("socketpair");
    }

    const int bsz = 64;
    char buf[bsz] = {'\0'};

    for (int i = 0; i < procnum; ++i)
    {
        pid = fork();
        if( pid < 0 )
        {
            perror( "fork" );
            return 1;
        }

        if( pid > 0 )
        {
            printf( "sub :%d process(%d) is created \n", i, pid );
            sprintf( buf, "msg:%d", i );
            write( pfd[1], buf, strlen(buf) );
            // 继续往后循环，创建子进程
        }
        else if( pid == 0 )
        {
            printf( "I am %d sub process(%d) \n", i, getpid() );
            sprintf( buf, "data%d", i );
            write( pfd[0], buf, strlen(buf) );
            
            // 子进程退出循环，处理自己的事情
            break;
        }
    }


    if( pid > 0 )
    {// 主进程

        // 监听子进程退出信号
        signal( SIGCHLD, sigcbk );

        // 设置为非阻塞
        int old_option = fcntl( pfd[0], F_GETFL );
        int new_option = old_option | O_NONBLOCK;
        fcntl( pfd[0], F_SETFL, new_option );

        while( !main_proc_stop )
        {
            memset( buf, '\0', bsz);
            read( pfd[0], buf, bsz );
            printf( "process(%d) got msg : '%s'\n", getpid(), buf );
            sleep(1);
        }
    }
    else
    {// 2 个子进程

        memset( buf, '\0', bsz );
        read( pfd[1], buf, bsz-1 );
        printf( "process(%d) got msg : '%s'\n", getpid(), buf );
        sleep(1);

    }

    close(pfd[0]);
    close(pfd[1]);

    return 0;
}

