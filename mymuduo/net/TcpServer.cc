#include "TcpServer.hh"
#include "../base/Logger.hh"
#include "TcpConnection.hh"

#include <string.h>

static EventLoop* CheckLoopNotNull(EventLoop* loop) {
    if (loop == nullptr) {
        /* mainloop为空不能初始化TcpServer 严重错误 */
        LOG_FATAL("CheckLoopNotNull mainloop is null \n");
    }
    return loop;
}


TcpServer::TcpServer(EventLoop* loop, 
const InetAddress& listenAddr, 
const std::string& nameArg, 
Option option) 
    : loop_(CheckLoopNotNull(loop)) /* loop_初始化时必须不为空 */
    , ipPort_(listenAddr.toIpPort())
    , name_(nameArg)
    , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
    , threadPool_(new EventLoopThreadPool(loop, name_))
    , connectionCallback_()
    , messageCallback_()
    , started_(0)
    , nextConnId_(1)
    {
        /* 新用户连接时执行TcpServer::newConnection 分配subloop */
        /* 运行在mainloop中 Acceptor::handleRead */
        acceptor_->setNewConnectionCallback(
            std::bind(&TcpServer::newConnection, 
            this, std::placeholders::_1, std::placeholders::_2));
}

/* 析构函数 关闭并释放所有的Tcp连接 */
TcpServer::~TcpServer() {
    for (auto& item : connections_) {
        TcpConnectionPtr conn(item.second); /* TcpConnectionPtr是shared_ptr */
        /* reset()方法会让item.second智能指针不再指向资源(TcpConnection)(引用计数-1) */
        item.second.reset();
        /* 销毁连接 */
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
        /* 离开作用域后conn指向的TcpConnection被析构 */
    }
}


/* 当有新的客户端连接 acceptor对应的channel会执行Acceptor::handleRead回调 过程中会执行该newConnection回调 */
/* 根据轮询算法选择一个subloop 唤醒subloop 把当前connfd封装为相应的channel 分发给subloop */
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
    /* 选择一个subloop来处理io事件 */
    EventLoop* ioLoop = threadPool_->getNextLoop();
    /* 新连接命名 */
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++ nextConnId_;
    std::string connName = name_ + buf;
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    /* 通过sockfd获取其绑定的本机ip和端口信息 */
    sockaddr_in local;
    bzero(&local, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0) {
        LOG_ERROR("TcpServer::newConnection getLocalAddr error\n");
    }
    InetAddress localAddr(local);
    /* 根据成功连接的sockfd创建TcpConnection连接对象 Socket Channel */
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    /* 保存创建的连接 */
    connections_[connName] = conn;
    /* 传入用户设置的回调 */
    /* 用户设置回调=>TcpServer=>TcpConnection=>Channel=>Poller=>notify Channel调用回调 */
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    /* 设置了如何关闭连接的回调 removeConnection */
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
    );

    /* 有新的TCP连接创建了TcpConnection后 直接调用TcpConnection::connectEstablisted方法 */
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

/* 设置subloop的数量 */
void TcpServer::setThreadNum(int numThreads) {
    threadPool_->setThreadNum(numThreads);
}

/* 启动服务器 */
void TcpServer::start() {
    if (started_ ++ == 0) {
        /* 防止TcpServer对象被start多次 */
        threadPool_->start(threadInitCallback_); /* subloop全部启动 */
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get())); /* 在mainloop上注册listenfd */
    }
    /* 调用完该方法 马上就会调用loop.loop()方法开启mainloop */
}

/* TcpConnection连接断开时 handleClose执行的回调 */
void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    loop_->runInLoop(std::bind(
        &TcpServer::removeConnectionInLoop, this, conn
    ));
}
/* removeConnection调用的 在对应subloop中执行 */
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
              name_.c_str(), conn->name().c_str());
    /* 在ConnectionMap中删除conn */
    connections_.erase(conn->name());
    /* 销毁TcpConnection::connectDestroyed */
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}