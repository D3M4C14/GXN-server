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

int ans = 100;
pthread_mutex_t amut;


void* work( void * arg )
{
    printf( "I am thread(%lx) \n", pthread_self() );

    int ret,s;

    while( true )
    {
        sched_yield();
        
        // 非阻塞 尝试等待锁
        ret = pthread_mutex_trylock( &amut );
        if( ret != 0 )
        {
            if( ret == EBUSY )
            {
                usleep(10);
                continue;
            }
            else
            {
                perror( "pthread_mutex_trylock" );
                return (void*)1;
            }
        }

        // 阻塞版本
        //pthread_mutex_lock( &amut );

        s = rand() & 0x0F;

        if( ans == 0 )
        {
            pthread_mutex_unlock( &amut );
            break;
        }

        ans -= s;
        
        if( ans <= 0 )
        {
            ans = 0;
            printf( "I am thread(%lx) finish work!!!\n", pthread_self() );
            
            pthread_mutex_unlock( &amut );
            break;
        }

        usleep(10);

        // 解锁 并且唤醒等待此锁的线程
        pthread_mutex_unlock( &amut );
    }

    pthread_exit( nullptr );

    return (void*)0;
}

int main( int argc, char* argv[] )
{

    int ret = 0;
    
    srand( (unsigned)time(nullptr) );

    pthread_mutexattr_t attr;
    pthread_mutexattr_init( &attr );

    // 进程内私有
    pthread_mutexattr_setpshared( &attr, PTHREAD_PROCESS_PRIVATE );

    // 类型为检错锁
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );

    // 初始化互斥锁
    pthread_mutex_init( &amut, &attr );

    for (int i = 0; i < thread_num; ++i)
    {
        ret = pthread_create( &threads[ i ], nullptr, work, nullptr );
        if( ret != 0 )
        {
            perror( "pthread_create" );
            return 1;
        }
    }

    for (int i = 0; i < thread_num; ++i)
    {
        pthread_join( threads[ i ], nullptr );
    }

    pthread_mutexattr_destroy( &attr );
    pthread_mutex_destroy( &amut );

    return 0;
}

