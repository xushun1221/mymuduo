#include "Acceptor.hh"
#include "../base/Logger.hh"
#include "InetAddress.hh"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


static int createNonblocking() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    /* listenfd创建失败是严重错误 */
    if (sockfd < 0) {
        LOG_FATAL("%s:%d listen socket create error:%d \n", __FILE__, __LINE__, errno);
    }
    return sockfd;
}


Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport) 
    : loop_(loop)
    , acceptSocket_(createNonblocking())
    , acceptChannel_(loop, acceptSocket_.fd())
    , listenning_(false)
    {
        acceptSocket_.setReuseAddr(true);
        acceptSocket_.setReusePort(reuseport);
        acceptSocket_.bindAddress(listenAddr); /* bind */
        /* 使用handleRead函数 处理新用户的连接 */
        acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
    /* baseloop不再监听新用户连接 */
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen() {
    listenning_ = true;
    acceptSocket_.listen(); /* listen */
    acceptChannel_.enableReading(); /* 注册到baseloop */
}

/* acceptChannel_收到新用户连接事件时 调用该函数 */
void Acceptor::handleRead() {
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr); /* 和新用户建立连接 */
    if (connfd >= 0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else {
            /* 如果一个客户端连接了 但没有相应的回调去处理 就关闭连接 */
            ::close(connfd);
        }
    } else {
        /* accept出错了 */
        LOG_ERROR("Acceptor::handleRead accept error:%d \n", errno);
        if (errno == EMFILE) {
            /* 该进程fd资源用尽 */
            LOG_ERROR("sockfd reached limited \n");
        }
    }
}