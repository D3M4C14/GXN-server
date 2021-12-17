#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main( int argc, char* argv[] )
{
    if( argc < 4 )
    {
        printf( "arg : file_name arg1 arg2 \n");
        return 1;
    }

    const char* file_name = argv[1];
    const char* arg1 = argv[2];
    const char* arg2 = argv[3];
    
    printf( "* runing %s : file_name:%s arg1:%s arg2:%s \n", basename( argv[0] ), file_name, arg1, arg2 );

    // if( access( file_name, F_OK | X_OK ) == -1 )
    // {
    //     perror( file_name );
    //     return 1;
    // }

    int pid = fork();
    if( pid == -1 )
    {
        perror( "fork" );
        return 1;
    }
    else if( pid == 0 )
    {
        execl( file_name, arg1, arg2, nullptr );
        printf( "execl '%s' failed try to environment variable find \n", file_name );
        execlp( file_name, arg1, arg2, nullptr );
        printf( "execlp '%s' failed\n", file_name );
        exit( 0 );
    }

    int stat;
    int ret = waitpid( pid, &stat, 0 );
    if( ret > 0 )
    {
        printf( "the process(%s) over and stat:%d\n", file_name, stat );
    }

    return 0;
}
