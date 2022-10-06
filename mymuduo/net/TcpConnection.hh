#ifndef   __TCPCONNECTION_HH_
#define   __TCPCONNECTION_HH_

#include "../base/noncopyable.hh"
#include "InetAddress.hh"
#include "Callbacks.hh"
#include "Buffer.hh"
#include "../base/Timestamp.hh"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;


/* TcpConnection代表一条已经建立的客户端连接 */
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop* loop,
                  const std::string& nameArg,
                  int sockfd,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }
    Buffer* inputBuffer() { return &inputBuffer_; }
    Buffer* outputBuffer() { return &outputBuffer_; }

    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }

    /* 发送数据 调用sendInLoop */
    void send(const std::string& buf);
    /* 关闭连接 调用shutdownInLoop */
    void shutdown();

    /* 设置回调 */
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) { 
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

    /* 连接建立 */
    void connectEstablished();
    /* 连接销毁 */
    void connectDestroyed();
    
private:
    /* TCP状态 */
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void setState(StateE state) { state_ = state; }

    /* 事件处理回调 */
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    /* 由当前loop发送数据 */
    void sendInLoop(const void* message, size_t len);
    /* 在当前loop中删除掉对应的channel */
    void shutdownInLoop();

    EventLoop* loop_;        /* 从属的subloop */
    const std::string name_;
    std::atomic_int state_;  /* TCP状态 */
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_; /* 高水位标记回调 */
    CloseCallback closeCallback_;
    size_t highWaterMark_; /* 高水位标记 */

    Buffer inputBuffer_;    /* 接收数据缓冲区 */
    Buffer outputBuffer_;   /* 发送数据缓冲区 */
};


#endif // __TCPCONNECTION_HH_