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
void sigcbk1( int sig )
{
    // 由于信号可能会中断其他系统操作,因此可能会覆盖其他调用的错误码,所以要备份和恢复错误码
    int lerrno = errno;
    printf( "sig:%d\n", sig );

    printf( "psignal info:\n" );
    psignal( sig, nullptr );

    errno = lerrno;
}

// 信号处理回调
void sigcbk2( int sig, siginfo_t* info, void* context )
{
    // 由于信号可能会中断其他系统操作,因此可能会覆盖其他调用的错误码,所以要备份和恢复错误码
    int lerrno = errno;

    printf( "psiginfo info:\n" );
    psiginfo( info, nullptr );

    printf( "sigcbk2 : sig(%d) val(%d) context(%d)\n", sig, info->si_int, *(int*)context );

    errno = lerrno;
}

// 添加信号监听
void addsig1( int sig )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = sigcbk1;

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

// 添加信号监听
void addsig2( int sig )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = ( void (*)(int) ) sigcbk2;

    //中断后继续被中断的系统操作(会意外的中断系统调用)
    //使用siginfo获得更详细的信息
    sa.sa_flags = SA_RESTART | SA_SIGINFO;

    // 设置所有信号
    sigfillset( &sa.sa_mask );

    int ret = sigaction( sig, &sa, nullptr );
    if( ret == -1 )
    {
        perror( "sigaction2" );
    }
}

void timercbk( int sig )
{
    printf( "timer:%d\n", sig );
    int sigd[] = { SIGTSTP, SIGCHLD, SIGHUP, SIGTERM, SIGQUIT, SIGUSR1 };
    int r = rand() % ( sizeof(sigd) / sizeof(int) );
    
    if( r & 1 )
    {
        kill( getpid(), sigd[r] );
    }
    else
    {
        sigqueue( getpid(), sigd[r], (sigval){123} );
    }
    
    alarm( 1 );
}

int main( int argc, char* argv[] )
{
    srand( (unsigned)time( nullptr ) );

    signal( SIGALRM, timercbk );

    alarm( 1 );

    signal( SIGCHLD, SIG_DFL );//默认行为
    signal( SIGTSTP, SIG_IGN );//忽略

    // 添加信号
    addsig1( SIGHUP );
    addsig1( SIGTERM );

    addsig2( SIGQUIT );
    addsig2( SIGUSR1 );

    while(true)
    {
        sleep(1);
    }
    return 0;
}

