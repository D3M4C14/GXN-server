#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

static void * sig_thread( void *arg )
{
    sigset_t *set = (sigset_t *) arg;
    int ret,sig,r;

    // 等待并处理信号，不必再需要其他的信号处理函数
    while( true )
    {

        r = rand() % 10;

        if( r > 3 )
        {
            timespec t = {0,1000};
            siginfo_t sinf;
            sig = sigtimedwait( set, &sinf, &t);
            if( sig < 0 )
            {
                if( errno != EAGAIN )
                {
                    perror( "sigtimedwait" );
                }
                continue;
            }
            printf( "thread [sigtimedwait] got signal(%d) val(%d)\n", sig, sinf.si_int );

            printf( "psiginfo info:\n" );
            psiginfo( &sinf, nullptr );
        }
        else if( r & 1 )
        {
            ret = sigwait( set, &sig );
            if( ret != 0 )
            {
                perror( "sigwait" );
            }
            printf( "thread [sigwait] got signal(%d)\n", sig );
        }
        else
        {
            siginfo_t sinf;
            sig = sigwaitinfo( set, &sinf );
            if( sig < 0 )
            {
                perror( "sigwaitinfo" );
            }
            printf( "thread [sigwaitinfo] got signal(%d) val(%d) code(%d)\n", sig, sinf.si_int, sinf.si_code );

            printf( "psiginfo info:\n" );
            psiginfo( &sinf, nullptr );
        }

    }

}

static void sigcbk( int arg )
{// 永远不会触发
    printf( "thread : %ld got sig : %d \n", pthread_self(), arg );
}

int main( int argc, char *argv[] )
{
    pthread_t thread;
    sigset_t set;
    int ret;

    // 整个进程的信号处理函数只有一个，多个进程设置同一个信号的处理函数会被覆盖
    // 因此这个信号处理函数将会无效
    signal( SIGQUIT, sigcbk );

    sigemptyset( &set );
    sigaddset( &set, SIGQUIT );
    sigaddset( &set, SIGUSR1 );

    // 主进程屏蔽信号
    ret = pthread_sigmask(SIG_BLOCK, &set, nullptr);
    if( ret != 0 )
    {
        perror( "pthread_sigmask" );
        return 1;
    }

    // 专门开一个线程来处理进程的所有信号
    ret = pthread_create( &thread, nullptr, &sig_thread, (void *) &set );
    if( ret != 0 )
    {
        perror( "pthread_create" );
        return 1;
    }
    printf( "sub thread with id: %ld\n", thread );

    int r;
    while(true)
    {
        sleep(1);

        r = rand() % 10;
        
        if( r & 1 )
        {
            kill( getpid(), SIGQUIT );
            kill( getpid(), SIGUSR1 );
        }
        else
        {
            sigqueue( getpid(), SIGQUIT, (sigval){123} );
            sigqueue( getpid(), SIGUSR1, (sigval){456} );
        }
    }

    return 0;
}

