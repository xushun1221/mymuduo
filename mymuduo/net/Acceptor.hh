#ifndef   __ACCEPTOR_HH_
#define   __ACCEPTOR_HH_

#include "../base/noncopyable.hh"
#include "Socket.hh"
#include "Channel.hh"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable {
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
    
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb) {
        newConnectionCallback_ = cb;
    }
    bool listenning() const { return listenning_; }
    void listen();

private:
    /* acceptChannel_收到新用户连接事件时 调用该函数 */
    void handleRead();

    EventLoop* loop_; /* 就是mainloop */
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_; /* 新用户连接时执行回调 将connfd打包为Channel 唤醒一个subloop处理connfd读写事件 */
    /* 该函数由TcpServer给出 */

    bool listenning_;

};


#endif // __ACCEPTOR_HH_