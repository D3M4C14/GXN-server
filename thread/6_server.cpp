#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>

pthread_mutex_t mut;

void* thread_func( void* arg )
{
    printf( "thread try get the lock\n" );
    pthread_mutex_lock( &mut );
    printf( "thread got the lock\n" );
    sleep( 2 );
    pthread_mutex_unlock( &mut );
    printf( "thread release the lock\n" );
    return nullptr;
}

void prepare()
{
    printf( "prepare try get the lock\n" );
    pthread_mutex_lock( &mut );
    printf( "prepare got the lock\n" );
}

void infork()
{
    pthread_mutex_unlock( &mut );
    printf( "infork release the lock\n" );
}

int main()
{
    pthread_mutex_init( &mut, nullptr );

    pthread_t tid;
    pthread_create( &tid, nullptr, thread_func, nullptr );

    // fork之前/fork之后父进程返回之前/fork之后子进程返回之前
    // 在fork之前 对所有要继承的锁都进行加锁(将会等待获得)
    // 在fork之后返回前 再把所有锁解开
    // 这样在子进程中这些锁都是已经解开可用的锁了
    pthread_atfork( prepare, infork, infork );

    sleep( 1 );

    // fork 将会继承父进程所有的锁但是不会继承额外创建的线程(单线程)
    int pid = fork();
    if( pid < 0 )
    {
        pthread_join( tid, nullptr );
        pthread_mutex_destroy( &mut );
        perror( "fork" );
        return 1;
    }
    else if( pid == 0 )
    {
        printf( "child process try get the lock\n" );
        pthread_mutex_lock( &mut );
        printf( "child process got the lock\n" );
        sleep(1);
        pthread_mutex_unlock( &mut );
        printf( "child process release the lock\n" );
        exit( 0 );
    }
    else
    {
        printf( "parent process try get the lock\n" );
        pthread_mutex_lock( &mut );
        printf( "parent process got the lock\n" );
        sleep( 1 );
        pthread_mutex_unlock( &mut );
        printf( "parent process release the lock\n" );
        waitpid( pid, nullptr, 0 );
    }

    pthread_join( tid, nullptr );

    pthread_mutex_destroy( &mut );

    return 0;
}
