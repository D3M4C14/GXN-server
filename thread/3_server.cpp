#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <semaphore.h>

const int thread_num = 4;

pthread_t threads[ thread_num ];

int ans = 100;
sem_t asem;


void* work( void * arg )
{
    int idx = *(int *)arg;
    printf( "thread(%lx) I am no.%d\n", pthread_self(), idx );

    int ret,s;

    while( true )
    {
        sched_yield();
        
        // 非阻塞 尝试等待信号值为非0值后进行减1操作
        ret = sem_trywait( &asem );
        if( ret != 0 )
        {
            if( errno == EAGAIN )
            {
                usleep(10);
                continue;
            }
            else
            {
                perror( "sem_trywait" );
                return (void*)1;
            }
        }

        // 阻塞版本
        //sem_wait( &asem );

        s = rand() & 0x0F;

        if( ans == 0 )
        {
            sem_post( &asem );
            break;
        }

        ans -= s;
        
        if( ans <= 0 )
        {
            ans = 0;
            printf( "thread(%lx) I am no.%d finish work!!!\n", pthread_self(), idx );
            sem_post( &asem );
            break;
        }

        usleep(10);

        // 信号值加1 并唤醒等待此信号量值的线程
        sem_post( &asem );
    }

    pthread_exit( nullptr );

    return (void*)0;
}

int main( int argc, char* argv[] )
{

    int ret = 0;
    
    srand( (unsigned)time(nullptr) );

    // 初始化信号量 进程内共享 初始值为1
    sem_init( &asem, 0, 1 );

    // 不可以直接传i变量偶尔会读取到同一个值
    int ai[thread_num];
    for (int i = 0; i < thread_num; ++i)
    {
        ai[i]=i+1;
        ret = pthread_create( &threads[ i ], nullptr, work, (void*)&ai[i] );
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

    sem_destroy( &asem );

    return 0;
}

