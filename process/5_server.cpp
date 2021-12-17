#include <sys/shm.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

// 考虑可移植性，名字应该以'/'开头
static const char* shm_fn = "/test_shm";
const int bufsz = 1024;

// 共享内存指针可被子进程继承使用
char *share_mem;

int main( int argc, char* argv[] )
{

    int shmfd = shm_open( shm_fn, O_CREAT | O_RDWR, 0666 );
    if( shmfd == -1 )
    {
        perror( "shm_open" );
        return 1;
    }

    int ret = ftruncate( shmfd, 2 * bufsz );
    if( ret == -1 )
    {
        perror( "ftruncate" );
        return 1;
    }

    share_mem = (char*)mmap( nullptr, 2 * bufsz, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0 );
    if( share_mem == MAP_FAILED )
    {
        perror( "mmap" );
        return 1;
    }
    close( shmfd );

    pid_t pid = fork();
    if( pid < 0 )
    {
        perror( "fork" );
        return 1;
    }
    else if( pid == 0 )
    {
        printf( "process(%d) write data : '%s'\n", getpid(), "abc123" );
        memset( share_mem+0*bufsz, '\0', bufsz);
        sprintf( share_mem+0*bufsz, "abc123" );

        sleep( 1 );

        printf( "process(%d) read data : '%s'\n", getpid(), share_mem+1*bufsz );

        munmap( (void*)share_mem,  2 * bufsz );
    }
    else if( pid > 0 )
    {
        printf( "process(%d) write data : '%s'\n", getpid(), "ABCzxc" );
        memset( share_mem+1*bufsz, '\0', bufsz);
        sprintf( share_mem+1*bufsz, "ABCzxc" );

        sleep( 1 );

        printf( "process(%d) read data : '%s'\n", getpid(), share_mem+0*bufsz );

        munmap( (void*)share_mem,  2 * bufsz );
    }

    waitpid( pid, nullptr, 0 );
    shm_unlink( shm_fn );

    return 0;
}
