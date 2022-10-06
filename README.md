# mymuduo

- xushun
- 2022/9/2

基于Reactor模型的多线程C++高性能网络库。

### 特性

- 使用C++11标准，OOP编程方式
- One Loop per Thread
- Multi-Reactors
- non-Blocking IO

### Requires

- Linux kernel version >= 2.6.28
- GCC version >= 4.8.1
- CMake version >= 2.5

### Build

运行`./autobuild.sh`以编译安装。

### 正在更新

- TcpClient客户端编程接口
- 对定时器的支持
- HTTP支持
- RPC支持
- QPS服务器性能测试
- 更丰富的编程示例


------

参考陈硕大神的[muduo](https://github.com/chenshuo/muduo)。