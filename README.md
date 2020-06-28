# shared_memory_communication

本项目通过编写内核驱动程序，实现了共享内存的读写，并以此为基础在应用层进行了封装，提供四个方法分别用于进程间建立连接、发送信息、接收信息、断开连接。

注意编译时需要对文件路径进行修改。



## driver

- shared：内核驱动程序
- Makefile：Makefile



## package

- communicate：应用层封装



## shell

- init.sh：加载模块
- check.sh：打印与模块相关的模块信息和设备信息
- compile.sh：编译相关的测试文件
- exit.sh：卸载模块



## test

- crash_test：用于验证竞争性条件的解决
- reader：读取特定设备（在本项目中即代表共享内存）的信息
- recevier1/2：从sender1/2接收信息并打印
- sender1：给receiver1发送信息，最后断开连接
- sender2：给receiver2发送信息，不断开连接
- regex.h：正则表达式使用的头文件