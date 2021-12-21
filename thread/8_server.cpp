#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

static pthread_once_t ponce = PTHREAD_ONCE_INIT;
pthread_key_t pkey;


static void destr_func( void *arg )
{
    int v = *(int*)arg;
    printf( "destory key(%d) value : %d\n", pkey, v );
}


static void init_func()
{
    pthread_key_create( &pkey, destr_func);
    printf( "call init func , only called once\n" );
}

void func()
{
    int *value = (int*)pthread_getspecific(pkey);
    printf( "func : key(%d) value : %d\n", pkey, *value );

    ++*value;

    pthread_setspecific( pkey, value );
}

static void * thread_func( void *arg  )
{
    int *data = (int*)arg;

    pthread_once( &ponce, init_func );

    pthread_setspecific( pkey, data );

    *data = *data * 10;

    func();

    data = (int*)pthread_getspecific(pkey);
    printf( "thread : key(%d) value : %d\n", pkey, *data );
    return nullptr;
}

int main(int argc, char *argv[])
{
    pthread_t pids[4];
    int data[4];
    for (int i = 0; i < 4; ++i)
    {
        data[i] = i+1;
        pthread_create( &pids[i], nullptr, thread_func, &data[i] );
    }

    for (int i = 0; i < 4; ++i)
    {
        pthread_join( pids[i], nullptr );
    }
    
    pthread_key_delete( pkey );

    return 0;
}

