* IO多路复用

* 编译运行命令
  `g++ -g -Wall -std=c++11 -o server 1_server.cpp`
  `./server`
  没有对应客户端文件的，可以直接用telnet进行测试  
  需要测试带外数据可以使用socket中的第1个客户端实例

* **1.** select 用法
  普通数据和带外数据、超时。

* **2.** poll 用法  
  普通数据和带外数据。

* **3.** epoll 用法  
  普通数据和带外数据、ET模式。