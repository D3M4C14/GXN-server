#include <sys/types.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

// 信号处理回调
void sigcbk( int sig )
{
    // 由于信号可能会中断其他系统操作,因此可能会覆盖其他调用的错误码,所以要备份和恢复错误码
    int lerrno = errno;
    printf("sig:%d\n", sig );
    errno = lerrno;
}

// 添加信号监听
void addsig( int sig )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = sigcbk;

    //中断后继续被中断的系统操作(会意外的中断系统调用)
    sa.sa_flags |= SA_RESTART;

    // 设置所有信号
    sigfillset( &sa.sa_mask );

    int ret = sigaction( sig, &sa, nullptr );
    if( ret == -1 )
    {
        perror( "sigaction" );
    }
}


int main( int argc, char* argv[] )
{
    srand( (unsigned)time( nullptr ) );

    // 设置屏蔽信号掩码
    sigset_t set;
    sigemptyset( &set );
    sigaddset( &set, SIGQUIT );
    sigaddset( &set, SIGUSR1 );

    sigdelset( &set, SIGUSR1 );

    printf( "sig:%d is member :%d\n", SIGUSR1, sigismember(&set, SIGUSR1) );

    sigprocmask( SIG_BLOCK, &set, nullptr );

    // 添加信号
    addsig( SIGHUP );
    addsig( SIGTERM );
    addsig( SIGQUIT );
    addsig( SIGUSR1 );

    while(true)
    {
        sleep(1);
        kill( getpid(), SIGHUP );
        kill( getpid(), SIGTERM );
        kill( getpid(), SIGQUIT );
        kill( getpid(), SIGUSR1 );

        // 查看被挂起的信号
        // **注意** fork 创建出来的进程会继承父进程的屏蔽信号掩码
        sigset_t pset;
        sigpending( &pset );

        printf( "sig:%d is pending :%d\n", SIGHUP, sigismember(&pset, SIGHUP) );
        printf( "sig:%d is pending :%d\n", SIGTERM, sigismember(&pset, SIGTERM) );
        printf( "sig:%d is pending :%d\n", SIGQUIT, sigismember(&pset, SIGQUIT) );
        printf( "sig:%d is pending :%d\n", SIGUSR1, sigismember(&pset, SIGUSR1) );

    }
    return 0;
}

