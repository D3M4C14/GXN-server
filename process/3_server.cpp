#include <sys/shm.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main( int argc, char* argv[] )
{
    // IPC_PRIVATE 代表 新创建一个信号量
    // 其他进程，尤其是父子进程都可以访问(由于历史原因，名称有点误导)
    // 0666 和 open 函数中的mode一致
    int shm_id = shmget( IPC_PRIVATE, 1024, 0666 & MAP_HUGETLB );
    if( shm_id == -1 )
    {
        perror( "shmget" );
        return 1;
    }

    pid_t pid = fork();
    if( pid < 0 )
    {
        perror( "fork" );
        return 1;
    }
    else if( pid == 0 )
    {
        char *shmptr = (char*) shmat( shm_id, nullptr, SHM_RND );
        if( shmptr == (void*)-1 )
        {
            perror( "shmat1" );
            return 1;
        }

        sprintf( shmptr, "abc123" );

        struct shm_info inf;
        shmctl( shm_id, SHM_INFO, (shmid_ds*)&inf );

        printf( "shm_info: \n%d\n%ld\n%ld\n%ld\n%ld\n%ld\n"
                    , inf.used_ids
                    , inf.shm_tot
                    , inf.shm_rss
                    , inf.shm_swp
                    , inf.swap_attempts
                    , inf.swap_successes );

        shmdt( shmptr );

        exit( 0 );
    }
    else
    {
        sleep( 1 );

        char *shmptr = ( char* ) shmat( shm_id, nullptr, SHM_RND );
        if( shmptr == MAP_FAILED )
        {
            perror( "shmat2" );
            return 1;
        }

        printf( "shm data: '%s'\n", shmptr );

        shmdt( shmptr );
    }

    waitpid( pid, nullptr, 0 );
    shmctl( shm_id, IPC_RMID, nullptr );
    return 0;
}
