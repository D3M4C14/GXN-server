#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// 消息队列的数据固定格式
// 数据长度可以自定义 发送和接收函数需要指定
template<int N>
struct msgdata
{
    long mtype;
    char mtext[N];
};

int main( int argc, char* argv[] )
{

    int msg_id = msgget( IPC_PRIVATE, 0666 );
    if( msg_id == -1 )
    {
        perror( "msgget" );
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
        const int msz = 32;
        struct msgdata<msz> msg;
        msg.mtype = 1;
        sprintf(msg.mtext,"msgdata");
        int ret = msgsnd( msg_id, (void*)&msg, msz, IPC_NOWAIT);
        if( ret == -1 )
        {
            perror( "msgsnd" );
            return 1;
        }
        printf( "process(%d) send msg : '%s' \n", getpid(), msg.mtext );
        exit( 0 );
    }
    else
    {
        sleep( 1 );
        const int msz = 64;
        struct msgdata<msz> msg;
        int ret = msgrcv( msg_id, (void*)&msg, msz, 1, IPC_NOWAIT);
        if( ret == -1 )
        {
            perror( "msgrcv" );
            return 1;
        }

        printf( "process(%d) got msg : '%s'\n", getpid(), msg.mtext );

    }

    waitpid( pid, nullptr, 0 );
    msgctl( msg_id, IPC_RMID, nullptr );
    return 0;
}
