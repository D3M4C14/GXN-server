# IO相关的优化的内核变量
进程打开的文件句柄数量超过了系统定义的值，就会报`too many files open`的错误提示。

* 查看进程打开了多少文件句柄 
句柄数量,进程ID  
`lsof -n |awk '{print $2}'|sort|uniq -c |sort -nr|more`  

* 查看系统限制  
`cat /proc/sys/fs/file-max`

* 查看用户限制  
`ulimit -n`

* 临时修改限制  
当前session有效，用户退出或者系统重新后恢复默认值  
H:硬限制 S：软限制  
`ulimit -HSn 4096`

* 修改用户限制  
在文件`/etc/security/limits.conf`末尾添加两行  
`* soft nofile 32768 #限制单个进程最大文件句柄数(到达此限制时系统报警)`  
`* hard nofile 65536 #限制单个进程最大文件句柄数(到达此限制时系统报错)`  
`*`为所有用户，可以改为需要指定的用户  

* 修改系统限制  
  一般不用修改，系统默认的值很大:`1048576`  

  临时修改，重启后恢复  
  `echo 655350 > /proc/sys/fs/file-max` 

  永久修改  
  在文件`/etc/sysctl.conf`末尾添加一行  
  `fs.file-max = 655350` 
  然后执行命令`sysctl –p`来刷新  