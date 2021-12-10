#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>


static void * sig_thread(void *arg)
{
    sigset_t *set = (sigset_t *) arg;
    int ret,sig;

    while( true )
    {
        // 等待并处理信号，不必再需要其他的信号处理函数
        ret = sigwait(set, &sig);
        if( ret != 0 )
        {
            perror( "sigwait" );
        }
        printf( "sig thread got signal : %d\n", sig );
    }

}

static void sigcbk( int arg )
{// 永远不会触发
    printf( "thread : %ld got sig : %d \n", pthread_self(), arg );
}

int main(int argc, char *argv[])
{
    pthread_t thread;
    sigset_t set;
    int ret;

    // 整个进程的信号处理函数只有一个，多个进程设置同一个信号的处理函数会被覆盖
    // 因此这个信号处理函数将会无效
    signal( SIGQUIT, sigcbk );

    sigemptyset( &set);
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

    while(true)
    {
        sleep(1);
        kill( getpid(), SIGQUIT );
        kill( getpid(), SIGUSR1 );
    }

    return 0;
}

