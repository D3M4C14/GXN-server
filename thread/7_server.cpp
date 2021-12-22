#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>


static void * sig_thread( void *arg  )
{
    sigset_t *set = (sigset_t *) arg;
    int ret,sig;

    while( true )
    {
        ret = sigwait(set, &sig);
        if( ret != 0 )
        {
            perror( "sigwait" );
        }
        printf( "sig thread got signal : %d\n", sig );
    }

}

int main(int argc, char *argv[])
{
    pthread_t tid,mytid = pthread_self();
    sigset_t set;
    int ret;

    sigemptyset( &set);
    sigaddset( &set, SIGQUIT );
    sigaddset( &set, SIGUSR1 );

    // 主进程屏蔽信号
    // 此后创建的线程都会继承这个屏蔽
    ret = pthread_sigmask(SIG_BLOCK, &set, nullptr);
    if( ret != 0 )
    {
        perror( "pthread_sigmask" );
        return 1;
    }

    // 专门开一个线程来处理进程的所有信号(对屏蔽的信号进行接收)
    ret = pthread_create( &tid, nullptr, &sig_thread, (void *) &set );
    if( ret != 0 )
    {
        perror( "pthread_create" );
        return 1;
    }
    printf( "sub thread with id: %ld\n", tid );

    while(true)
    {
        // 对进程发送信号
        sleep(1);
        printf( "signal to process\n");
        kill( getpid(), SIGQUIT );
        kill( getpid(), SIGUSR1 );

        // 对线程发送信号
        sleep(1);
        printf( "signal to sub thread\n");
        pthread_kill( tid, SIGQUIT );
        pthread_sigqueue( tid, SIGUSR1, (sigval){123} );

        // 主线程已经屏蔽了信号,因此不会响应此信号发送
        sleep(1);
        printf( "signal to main thread\n");
        pthread_kill( mytid, SIGQUIT );
        pthread_kill( mytid, SIGUSR1 );

    }

    pthread_join( tid, nullptr );

    return 0;
}

