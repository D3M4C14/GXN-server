* 高级 IO 操作

* 编译运行命令
  `g++ -g -Wall -std=c++11 -o server 1_server.cpp`
  `./server`
  没有对应客户端文件的，可以直接用telnet进行测试

* **1.**基础的 管道 用法  
  dup splice tee 等。

* **2.** 文件IO 用法  
  mmap sendfile writev 等。