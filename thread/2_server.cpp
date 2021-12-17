#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

const int thread_num = 4;

pthread_t threads[ thread_num ];

void lose( void * arg )
{
    printf( "thread(%lx) canceled(%d)\n", pthread_self(), *(int *)arg );
}


void* work( void * arg )
{
    int ans = *(int *)arg;
    printf( "thread(%lx) find:%d\n", pthread_self(), ans );

    int idx = 0;

    // 注册一个取消函数回调 压入栈
    pthread_cleanup_push( lose, (void*)&idx );

    // 设置取消类型和状态
    pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, nullptr );
    pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, nullptr );

    int s = rand() & 0xFFFFF;

    while( true )
    {
        sched_yield();
        
        ++idx;

        if( s < ans)
        {
            s = s * 2;
        }
        else if( s > ans)
        {
            --s;
        }
        else
        {
            break;
        }
        pthread_testcancel();
    }

    pthread_t sid = pthread_self();
    for (int i = 0; i < thread_num; ++i)
    {
        if( threads[ i ] != sid ) pthread_cancel( threads[ i ] );
    }

    printf( "thread(%ld) found ans idx : %d \n", sid, idx );

    // 取消所有取消函数回调 清空栈
    // pthread_join的返回值也将被清理 导致返回的结果是0
    pthread_cleanup_pop( 0 );

    pthread_exit( (void*)&idx );

    return (void*)0;
}

int main( int argc, char* argv[] )
{

    int ret = 0;
    
    srand( (unsigned)time(nullptr) );

    int ans = rand() & 0xFFFFF;

    for (int i = 0; i < thread_num; ++i)
    {
        ret = pthread_create( &threads[i], nullptr, work, (void*)&ans );
        if( ret != 0 )
        {
            perror( "pthread_create" );
            return 1;
        }
    }

    for (int i = 0; i < thread_num; ++i)
    {
        int * retval;

        pthread_join( threads[i], (void**)&retval );

        // if( retval != (void*)-1 )printf( "thread(%lx) found ans idx : %d \n", threads[ i ] , *retval );

    }

    return 0;
}

