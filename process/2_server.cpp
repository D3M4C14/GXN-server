#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

//semctl 函数的第4个用户自定义参数样式
union semun
{
    int val;                  
    struct semid_ds* buf;     
    unsigned short int* array;
    struct seminfo* __buf;    
};

int pv( int sem_id, int sn, int op, int flg = SEM_UNDO )
{
    struct sembuf sem_b;
    sem_b.sem_num = sn;
    sem_b.sem_op = op;
    sem_b.sem_flg = flg;
    return semop( sem_id, &sem_b, 1 );
}

int pvs( int sem_id, int sns[], int ops[], int len, int flg = SEM_UNDO )
{
    struct sembuf sem_b[len];
    for (int i = 0; i < len; ++i)
    {
        sem_b[i].sem_num = sns[i];
        sem_b[i].sem_op = ops[i];
        sem_b[i].sem_flg = flg;
    }
    return semop( sem_id, sem_b, len );
}

int main( int argc, char* argv[] )
{
    // IPC_PRIVATE 代表 新创建一个信号量
    // 其他进程，尤其是父子进程都可以访问(由于历史原因，名称有点误导)
    // 0666 和 open 函数中的mode一致
    int sem_id = semget( IPC_PRIVATE, 4, 0666 );

    // 初始化信号集
    union semun su;

    // 信号集所有信号
    unsigned short sa[] = {0,1,0,0};
    su.array = sa;
    semctl( sem_id, 0, SETALL, su );

    // 第一个信号设置为1
    su.val = 1;
    semctl( sem_id, 0, SETVAL, su );

    pid_t pid = fork();
    if( pid < 0 )
    {
        perror( "fork" );
        return 1;
    }
    else if( pid == 0 )
    {
        printf( "child try to get binary sem(0)\n" );
        pv( sem_id, 0, -1 );
        printf( "child get the sem(0) and would release it after 2 seconds\n" );
        sleep( 2 );
        printf( "child release sem(0)\n" );
        pv( sem_id, 0, 1 );

        sleep( 1 );
        
        while( true )
        {
            printf( "child try to get binary sem(1)\n" );
            // 非阻塞
            int ret = pv( sem_id, 1, -1, IPC_NOWAIT );
            if( ret == 0 )
            {
                printf( "child get the sem(1) and would release it after 2 seconds\n" );
                sleep( 1 );
                printf( "child release sem(1)\n" );
                pv( sem_id, 1, 1 );
                break;
            }
            usleep( 200000 );
        }
        

        exit( 0 );
    }
    else
    {
        printf( "parent try to get binary sem(0)\n" );
        pv( sem_id, 0, -1 );
        printf( "parent get the sem(0) and would release it after 2 seconds\n" );
        sleep( 2 );
        printf( "parent release sem(0)\n" );
        pv( sem_id, 0, 1 );

        sleep( 2 ); // 确保子线程先获取信号量
        
        while( true )
        {
            printf( "parent waiting binary sem(1) to zero\n" );
            // 非阻塞 将会等到 子进程拿到sem(1) 才会变成0
            int ret = pv( sem_id, 1, 0, IPC_NOWAIT );
            if( ret == 0 )
            {
                printf( "parent waited binary sem(1) is zero\n" );

                printf( "parent try to get binary sem(0,1)\n" );
                int sea[] = {0,1};
                int soa[] = {-1,-1};
                pvs( sem_id, sea, soa, 2 );
                
                printf( "parent get the sem(0,1) and would release it after 2 seconds\n" );
                sleep( 2 );

                printf( "parent release sem(0,1)\n" );
                soa[0] = 1;
                soa[1] = 1;
                pvs( sem_id, sea, soa, 2 );

                break;
            }
            usleep( 500000 );
        }

    }

    waitpid( pid, NULL, 0 );
    semctl( sem_id, 0, IPC_RMID );
    return 0;
}
