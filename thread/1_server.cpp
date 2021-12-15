#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

void* thread_func( void * arg )
{
    printf( "I am thread got arg : %d\n", *(int*)arg );
    while( true )
    {
        sched_yield();
        break;
    }

    pthread_t tid = pthread_self();

    pthread_exit( (void*)&tid );

    return nullptr;
}

int main( int argc, char* argv[] )
{

    int arg = getpid();
    pthread_t tid;
    
    pthread_attr_t attr;
    pthread_attr_init( &attr );

    // 线程尾部对战保护区域大小(尾部额外分配n字节空间)
    pthread_attr_setguardsize( &attr, 32 );

    // 调度参数
    struct sched_param param;
    param.__sched_priority = 1;
    pthread_attr_setschedparam( &attr, &param );

    // 调度策略
    pthread_attr_setschedpolicy( &attr, SCHED_OTHER );

    int ret = pthread_create( &tid, &attr, thread_func, (void*)&arg );
    if( ret != 0 )
    {
        perror( "pthread_create" );
        return 1;
    }

    pthread_attr_destroy( &attr );

    pthread_t * retval;

    pthread_join( tid, (void**)&retval );

    printf("got thread retval : ptr(%p) val(%ld)\n", retval , *retval );

    return 0;
}

