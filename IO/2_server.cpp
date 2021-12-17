#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/mman.h>

int main( int argc, char* argv[] )
{
    if( argc <= 3 )
    {
        printf( "arg: ip port datafile\n" );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );
    const char* file_name = argv[3];

    char* file_buf;
    int file_fd;

    // 文件处理
    struct stat file_stat;
    if( stat( file_name, &file_stat ) < 0 )
    {
        printf( "datafile : %s stat error!\n", file_name );
        return 1;
    }

    if( file_stat.st_mode & S_IROTH )
    {
        file_fd = open( file_name, O_RDONLY );
        file_buf = new char [ file_stat.st_size + 1 ];
        memset( file_buf, '\0', file_stat.st_size + 1 );
        if ( read( file_fd, file_buf, file_stat.st_size ) < 0 )
        {
            printf( "datafile : %s read error!\n", file_name );
            delete [] file_buf;
            close( file_fd );
            return 1;
        }
    }
    else
    {
        printf( "datafile : %s mode error!\n", file_name );
        return 1;
    }

    // mmap 必须 O_RDWR 如果是O_WRONLY 则会权限错误，mmap把文件的内容读到内存时隐含了一次读取操作
    const char* mmap_file_name = "mmap.txt";
    int mmap_file_fd = open( mmap_file_name, O_CREAT | O_RDWR | O_TRUNC, 0666 );
    if( mmap_file_fd < 0 )
    {
        printf( "mmap file : %s creat error!\n", mmap_file_name );
        return 1;
    }
    const int mmapsz = 1024;
    // 扩容 当文件内容小于你要操作的空间需要进行扩容
    ftruncate( mmap_file_fd, mmapsz );
    char* mmap_file_ptr = (char*)mmap( nullptr, mmapsz, PROT_READ | PROT_WRITE
                                        , MAP_SHARED, mmap_file_fd, 0);
    if(mmap_file_ptr == MAP_FAILED)
    {
        printf( "mmap : %s mmap error!\n", mmap_file_name );
        perror( "mmap" );
        return 1;
    }

    // 关闭了mmap的fd也不会有影响
    close( mmap_file_fd );

    printf( "* runing %s : %s:%d file:%s mmap file:%s \n"
                    , basename( argv[0] ), ip, port, file_name, mmap_file_name);

    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    int sock = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sock >= 0 );

    int ret = bind( sock, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );

    ret = listen( sock, 5 );
    assert( ret != -1 );

    struct sockaddr_in client;
    socklen_t clen = sizeof( client );
    int connfd = accept( sock, ( struct sockaddr* )&client, &clen );
    if ( connfd < 0 )
    {
        printf( "errno is: %d\n", errno );
    }
    else
    {
        // 客户端链接信息
        char remote[INET_ADDRSTRLEN];
        printf( "client connected : %s:%d\n", 
                    inet_ntop( AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN ), ntohs( client.sin_port ) );

        const int bsz = 4096;
        char buffer[ bsz ];

        while(true)
        {
            
            usleep(100000);

            memset( buffer, '\0', bsz );
            ret = recv( connfd, buffer, bsz-1, MSG_DONTWAIT );
            if( ret > 0 ){

                int blen = strlen( buffer );
                if( blen > 1 && buffer[0] == 'A')
                {
                    struct iovec iv[2];
                    iv[ 0 ].iov_base = buffer;
                    iv[ 0 ].iov_len = strlen( buffer );
                    iv[ 1 ].iov_base = file_buf;
                    iv[ 1 ].iov_len = file_stat.st_size;
                    ret = writev( connfd, iv, 2 );
                }
                else if( blen > 1 && buffer[0] == 'B')
                {
                    //前面的read使得fd的偏移已经到文件末尾了，必须重置到文件头才行
                    lseek( file_fd, 0, SEEK_SET );
                    sendfile( connfd, file_fd, nullptr, file_stat.st_size );
                }
                else if( blen > 1 && buffer[0] == 'C' )
                {
                    snprintf( mmap_file_ptr,blen, "recv:%s\n", buffer);
                    ret = msync( mmap_file_ptr, mmapsz, MS_SYNC );
                    if( ret == -1 )
                    {
                        perror( "msync" );
                    }
                }
                else if( blen > 1 && buffer[0] == 'Q')
                {
                    break;
                }

            }

        }

        close( connfd );
    }

    delete [] file_buf;
    close( file_fd );
    munmap( mmap_file_ptr, mmapsz );

    close( sock );
    return 0;
}

