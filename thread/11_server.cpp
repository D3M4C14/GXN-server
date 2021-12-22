#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

pthread_barrier_t barrier;

static void * thread_func( void *arg  )
{
    int t = *(int*)arg;

    sleep(t);
    printf( "no.%d ready ok\n",t );

    pthread_barrier_wait( &barrier );

    printf( "no.%d run\n",t );

    return nullptr;
}

int main( int argc, char *argv[] )
{
    const int num = 3;
    pthread_t pids[num];
    
    pthread_barrier_init( &barrier, nullptr, num+1 );

    int t[num];
    for (int i = 0; i < num; ++i)
    {
        t[i] = i + 1;
        pthread_create( &pids[i], nullptr, &thread_func, &t[i] );
    }
    
    sleep(5);
    printf( "start\n" );

    pthread_barrier_wait( &barrier );

    for (int i = 0; i < num; ++i)
    {
        pthread_join( pids[i], nullptr );
    }
    
    pthread_barrier_destroy( &barrier );

    return 0;
}

