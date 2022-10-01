#ifndef   __TCPSERVER_HH_
#define   __TCPSERVER_HH_

#include "noncopyable.hh"
#include "EventLoop.hh"
#include "EventLoopThread.hh"
#include "EventLoopThreadPool.hh"
#include "Acceptor.hh"
#include "InetAddress.hh"
#include "Callbacks.hh"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>


/* TcpServer服务器类 服务器编程的入口类 */
class TcpServer : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; /* EventLoopThread创建时对loop进行操作的回调类型 */
    enum Option { kNoReusePort, kReusePort };

    TcpServer(EventLoop* loop, 
              const InetAddress& listenAddr, 
              const std::string& nameArg, 
              Option option = kNoReusePort);
    ~TcpServer();

    /* 设置各种回调 */
    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    /* 设置subloop的数量 */
    void setThreadNum(int numThreads);
    /* 启动服务器 */
    void start();
private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    /* 连接相关 */
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

/* 组件 */
    EventLoop* loop_; /* baseloop是由用户传入的 */
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_; /* 运行在mainloop监听新连接 */
    std::shared_ptr<EventLoopThreadPool> threadPool_; /* one loop per thread */
/* 回调 */
    ConnectionCallback connectionCallback_; /* 处理新连接用户的回调 */
    MessageCallback messageCallback_;       /* 处理已连接用户的读写事件的回调 */
    WriteCompleteCallback writeCompleteCallback_; /* 消息发送完成后的回调 */
    ThreadInitCallback threadInitCallback_; /* 线程创建时对loop进行处理的回调 */

    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_; /* 保存所有的连接 */
};


#endif // __TCPSERVER_HH_