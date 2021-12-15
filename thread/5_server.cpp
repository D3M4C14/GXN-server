#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

const int thread_num = 4;

pthread_t threads[ thread_num ];

pthread_t wtid;

int ans = 100;
pthread_mutex_t amut;
pthread_cond_t acond;


void* work( void * arg )
{
    printf( "I am thread(%lx) \n", pthread_self() );

    int ret,s;
    
    pthread_t tid = pthread_self();

    while( true )
    {
        sched_yield();
        
        pthread_mutex_lock( &amut );

        // 得到锁后发现已经结束了
        if( ans == 0 )
        {
            pthread_mutex_unlock( &amut );
            break;
        }
        
        // 可能有多个线程在此处等待
        // 在等待时 锁被解开 当成功返回时会被加锁
        timespec t = {0,100};
        ret = pthread_cond_timedwait( &acond, &amut, &t );
        if( ret != 0 )
        {
            pthread_mutex_unlock( &amut );
            usleep(10);
            continue;
        }

        // 无超时阻塞版本
        //pthread_cond_wait( &acond, &amut );

        // 得到通知后发现已经结束了
        if( ans == 0 )
        {
            pthread_mutex_unlock( &amut );
            break;
        }

        if( tid == wtid )
        {

            s = rand() & 0x0F;


            ans -= s;
            
            if( ans <= 0 )
            {
                ans = 0;
                printf( "I am thread(%lx) finish work!!!\n", pthread_self() );

                pthread_mutex_unlock( &amut );

                // 通知所有等待的线程已经结束了
                pthread_cond_broadcast( &acond );

                break;
            }

        }

        usleep( 10 );
        pthread_mutex_unlock( &amut );
        pthread_cond_signal( &acond );
    }

    pthread_exit( nullptr );

    return (void*)0;
}

int main( int argc, char* argv[] )
{

    int ret = 0;
    
    srand( (unsigned)time(nullptr) );

    // 初始化

    pthread_condattr_t attr;
    pthread_condattr_init( &attr );

    // 进程内私有
    pthread_condattr_setpshared( &attr, PTHREAD_PROCESS_PRIVATE );

    // 设置使用时钟类型
    pthread_condattr_setclock( &attr, CLOCK_MONOTONIC );

    pthread_cond_init( &acond, &attr );
    amut = PTHREAD_MUTEX_INITIALIZER;

    for (int i = 0; i < thread_num; ++i)
    {
        ret = pthread_create( &threads[ i ], nullptr, work, nullptr );
        if( ret != 0 )
        {
            perror( "pthread_create" );
            return 1;
        }
    }

    while( ans > 0 )
    {

        wtid = threads[ rand() % thread_num ];

        // 发出信号通知线程可以开始了
        pthread_cond_broadcast( &acond );

        sched_yield();
    }


    for (int i = 0; i < thread_num; ++i)
    {
        pthread_join( threads[ i ], nullptr );
    }

    pthread_mutex_destroy( &amut );
    pthread_cond_destroy( &acond );

    return 0;
}

