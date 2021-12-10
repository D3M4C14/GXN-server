#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>

static const int sem_key = 0x3af7e1c5;
static const char* shm_fn = "/tf_shm";
const int bufsz = 1024;

static bool stop = false;

union semun
{
    int val;                  
    struct semid_ds* buf;     
    unsigned short int* array;
    struct seminfo* __buf;    
};

void timercbk( int sig )
{
    stop = true;
}

int pv( int sem_id, int op )
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = op;
    sem_b.sem_flg = SEM_UNDO;
    return semop( sem_id, &sem_b, 1 );
}

int main( int argc, char* argv[] )
{

    signal( SIGALRM, timercbk );
    alarm( 10 );

    // 唯一排他创建信号
    int sem_id = semget( sem_key, 1, 0666 | IPC_CREAT | IPC_EXCL );
    if( sem_id > 0 )
    {// 若创建成功进行 初始化
        union semun su;
        su.val = 1;
        semctl( sem_id, 0, SETVAL, su );
    }
    else 
    {// 若已经创建则直接获取使用
        if( errno == EEXIST )
        {
            sem_id = semget( sem_key, 1, 0666 );
            if( sem_id < 1 )
            {
                perror( "semget2" );
                return 1;
            }
        }
        else
        {
            perror( "semget1" );
            return 1;
        }
    }

    int shmfd = shm_open( shm_fn, O_CREAT | O_RDWR, 0666 );
    if( shmfd == -1 )
    {
        perror( "shm_open" );
        return 1;
    }

    int ret = ftruncate( shmfd, bufsz );
    if( ret == -1 )
    {
        perror( "ftruncate" );
        return 1;
    }

    char *share_mem = (char*)mmap( nullptr, bufsz, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0 );
    if( share_mem == MAP_FAILED )
    {
        perror( "mmap" );
        return 1;
    }
    close( shmfd );

    srand( (unsigned)time( nullptr ) );

    int pid = getpid();

    while( !stop )
    {
        int r = rand() % ( 10 );
        usleep( 100000 * r );

        ret = pv( sem_id, -1 );
        if( ret == -1 )
        {// 可能其他进程把信号移除了
            break;
        }
        memset( share_mem, '\0', bufsz );
        sprintf( share_mem, "data%d%d%d", r,r,r );
        printf( "process(%d) write data : '%s'\n", pid, share_mem );
        usleep( 400000 );
        ret = pv( sem_id, 1 );
        if( ret == -1 )
        {// 可能其他进程把信号移除了
            break;
        }

        r = rand() % ( 10 );
        usleep( 200000 * r );

        printf( "process(%d) read data : '%s'\n", getpid(), share_mem );
    }

    munmap( (void*)share_mem, bufsz );

    shm_unlink( shm_fn );

    semctl( sem_id, 0, IPC_RMID );

    return 0;
}
