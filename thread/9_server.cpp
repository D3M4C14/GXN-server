#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

int data = 1024;

pthread_spinlock_t spinlock;

static void * thread_func( void *arg  )
{
    bool ok;
    while( true )
    {
        ok = false;

        pthread_spin_lock( &spinlock );
        if ( data > 0 )
        {
            --data;
        }
        if ( data == 0 )
        {
            printf( "ok\n" );
            ok = true;
        }

        // 小心 这里如果改大导致有更多的其他线程进行忙等待，会极其耗费cpu资源 (阻塞总线)
        usleep(0);

        pthread_spin_unlock( &spinlock );

        usleep(0);

        if( ok ) break;
    }
    return nullptr;
}

int main( int argc, char *argv[] )
{
    pthread_t pids[20];
    
    pthread_spin_init( &spinlock, 0 );

    for (int i = 0; i < 20; ++i)
    {
        pthread_create( &pids[i], nullptr, &thread_func, nullptr );
    }

    for (int i = 0; i < 20; ++i)
    {
        pthread_join( pids[i], nullptr );
    }

    pthread_spin_destroy( &spinlock );

    return 0;
}

