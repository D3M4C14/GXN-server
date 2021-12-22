#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

int data = 8;

pthread_rwlock_t rwlock;

static void * thread_func( void *arg  )
{
    bool ok;
    while( true )
    {
        ok = false;

        pthread_rwlock_rdlock( &rwlock );

        // int ret = pthread_rwlock_tryrdlock( &rwlock );
        // if ( ret != 0 )
        // {
        //     if ( ret == EBUSY )
        //     {
        //         sleep(1);
        //         continue;
        //     }
        //     else
        //     {
        //         perror( "pthread_rwlock_tryrdlock" );
        //         break;
        //     }
        // }

        // timespec t = {0,100};
        // int ret = pthread_rwlock_timedrdlock( &rwlock, &t );
        // if ( ret != 0 )
        // {
        //     if ( ret == ETIMEDOUT )
        //     {
        //         sleep(1);
        //         continue;
        //     }
        //     else
        //     {
        //         perror( "pthread_rwlock_timedrdlock" );
        //         break;
        //     }
        // }

        printf( "read:%d\n", data );
        if ( data == 0 )
        {
            ok = true;
        }

        sleep(1);

        pthread_rwlock_unlock( &rwlock );

        usleep(10000);

        if( ok ) break;
    }
    return nullptr;
}

int main( int argc, char *argv[] )
{
    pthread_t pids[3];
    
    pthread_rwlock_init( &rwlock, nullptr );

    for (int i = 0; i < 3; ++i)
    {
        pthread_create( &pids[i], nullptr, &thread_func, nullptr );
    }
    
    // 子线程连续抢占读写锁的时候，可能会导致写锁一直没有机会获取
    bool ok;
    while( true )
    {
        ok = false;

        pthread_rwlock_wrlock( &rwlock );
        if ( data > 0 )
        {
            --data;
        }
        printf( "write:%d\n", data );
        if ( data == 0 )
        {
            printf( "ok\n" );
            ok = true;
        }

        sleep(1);

        pthread_rwlock_unlock( &rwlock );

        usleep(10);

        if( ok ) break;
    }


    for (int i = 0; i < 3; ++i)
    {
        pthread_join( pids[i], nullptr );
    }
    
    pthread_rwlock_destroy( &rwlock );

    return 0;
}

