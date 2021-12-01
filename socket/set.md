# socket相关可优化的内核变量
修改位于文件夹`/proc/sys/`下面的对应文件项进行临时修改(重启失效)  
或直接永久修改`/etc/sysctl.conf`文件，修改后需要执行命令`sysctl –p`来刷新  
这里都给出了建议设置的值，可以根据情况适当调整。如果不清楚的地方和了解更多的内容可以查阅先关文档或他人的相关文章。  

* SYN Cookies 
防止 SYN Flood 攻击，被故意大量不断发送伪造的SYN报文，那么服务器就会分配大量注定无用的资源。  
**0**表示关闭SYN Cookies  
**1**表示在新连接压力比较大时启用SYN Cookies(默认)  
**2**表示始终使用SYN Cookies  
`cat /proc/sys/net/ipv4/tcp_syncookies`  
`echo 1 > /proc/sys/net/ipv4/tcp_syncookies`  
`net.ipv4.tcp_syncookies = 1`  

* syns queue  
保存半连接状态的请求队列长度，默认值512，超过这个数量，系统将不再接受新的TCP连接请求。  
`cat /proc/sys/net/ipv4/tcp_max_syn_backlog`  
`echo 8192 > /proc/sys/net/ipv4/tcp_max_syn_backlog`  
`net.ipv4.tcp_max_syn_backlog = 8192`  

* accept queue  
定义了系统中每一个端口最大的监听队列的长度，这是个全局的参数。  
保存全连接状态的请求队列长度，默认值128，listen函数的backlog参数与系统参数somaxconn，取二者较小值。  
如果队列满了，将发送ECONNREFUSED错误信息Connection refused到client。  
`cat /proc/sys/net/core/somaxconn`  
`echo 8192 > /proc/sys/net/core/somaxconn`  
`net.core.somaxconn = 8192`  

* 进入包的最大设备队列  
在每个网络接口接收数据包的速率比内核处理这些包的速率快时，允许送到队列的数据包的最大数目。  
对重负载服务器而言可以适当提高  
`cat /proc/sys/net/core/netdev_max_backlog`  
`echo 16384 > /proc/sys/net/core/netdev_max_backlog`  
`net.core.netdev_max_backlog = 16384`  

* tcp 失败重传次数  
默认值15,意味着重传15次才彻底放弃,可减少到5,以尽早释放内核资源  
`cat /proc/sys/net/ipv4/tcp_retries2`  
`echo 5 > /proc/sys/net/ipv4/tcp_retries2`  
`net.ipv4.tcp_retries2 = 5`  

* tcp KeepAlive 相关  
1.`tcp_keepalive_time`:TCP发送keepalive探测消息的间隔时间（秒），用于确认TCP连接是否有效。  
2.`tcp_keepalive_intvl`:探测消息未获得响应时，重发该消息的间隔时间（秒）。  
3.`tcp_keepalive_probes`:在认定TCP连接失效之前，最多发送多少个keepalive探测消息。  
如果某个tcp连接在idle 2个小时后,内核才发起 probe 如果 probe 9次(每次75秒)不成功,内核才彻底放弃。  
可适当调低： 1800 30 3  
`cat /proc/sys/net/ipv4/tcp_keepalive_time`  
`echo 1800 > /proc/sys/net/ipv4/tcp_keepalive_time`  
`net.ipv4.tcp_keepalive_time = 1800`  

* tcp 重用  
允许将 TIME-WAIT sockets 重新用于新的TCP连接，默认为0，表示关闭  
`cat /proc/sys/net/ipv4/tcp_tw_reuse`  
`echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse`  
`net.ipv4.tcp_tw_reuse = 1`  

* tcp 快速回收  
将 TIME_WAIT sockets 快速回收，默认为0，表示关闭  
`cat /proc/sys/net/ipv4/tcp_tw_recycle`  
`echo 1 > /proc/sys/net/ipv4/tcp_tw_recycle`  
`net.ipv4.tcp_tw_recycle = 1`  

* tcp FIN-WAIT-2 状态时间  
表示如果套接字由本端要求关闭，这个参数决定了它保持在FIN-WAIT-2状态的时间。  
`cat /proc/sys/net/ipv4/tcp_fin_timeout`  
`echo 30 > /proc/sys/net/ipv4/tcp_fin_timeout`  
`net.ipv4.tcp_fin_timeout = 30`  

* tcp 对外端口范围  
表示用于对外连接的端口范围,可以适当调宽  
`cat /proc/sys/net/ipv4/ip_local_port_range`  
`echo 1024 65535 > /proc/sys/net/ipv4/ip_local_port_range`  
`net.ipv4.ip_local_port_range = 1024 65535`  

* tcp TIME_WAIT 状态套接字的最大数量  
系统同时保持TIME_WAIT套接字的最大数量，如果超过这个数字，TIME_WAIT套接字将立刻被清除并打印警告信息。  
可适当增大该值，但不建议减小。  
`cat /proc/sys/net/ipv4/tcp_max_tw_buckets`  
`echo 200000 > /proc/sys/net/ipv4/tcp_max_tw_buckets`  
`net.ipv4.tcp_max_tw_buckets = 200000`  

* tcp 读缓冲区  
数值(最小,默认,最大)  
`cat /proc/sys/net/ipv4/tcp_rmem`  
`echo 873200 1746400 3492800 > /proc/sys/net/ipv4/tcp_rmem`  
`net.ipv4.tcp_rmem = 873200 1746400 3492800`  

* tcp 写缓冲区  
数值(最小,默认,最大)  
`cat /proc/sys/net/ipv4/tcp_wmem`  
`echo 873200 1746400 3492800 > /proc/sys/net/ipv4/tcp_wmem`  
`net.ipv4.tcp_wmem = 873200 1746400 3492800`  

* tcp 内存  
数值(无内存压力,进入pressure模式,拒绝分配socket),内存单位是页,而不是字节  
`cat /proc/sys/net/ipv4/tcp_mem`  
`echo 78643200 104857600 157286400 > /proc/sys/net/ipv4/tcp_mem`  
`net.ipv4.tcp_mem = 78643200 104857600 157286400`  

* 收发套接字缓冲区大小  
tcp,udp等套接字缓冲区最大可设置值的**一半**(系统会对其翻倍)  
共有`rmem_default rmem_max wmem_default wmem_max`:  
`cat /proc/sys/net/core/rmem_default`  
`echo 16777216 > /proc/sys/net/core/rmem_default`  
`net.core.rmem_default = 16777216`  

* 套接字最大缓冲区大小  
表示每个套接字所允许的最大缓冲区的大小。  
`cat /proc/sys/net/core/optmem_max`  
`echo 81920 > /proc/sys/net/core/optmem_max`  
`net.core.optmem_max = 81920`  
