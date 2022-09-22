#include "TcpServer.hh"
#include "Logger.hh"



EventLoop* CheckLoopNotNull(EventLoop* loop) {
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
    , nextConnId_(1)
    {
        /* 新用户连接时执行TcpServer::newConnection 分配subloop */
        /* 运行在mainloop中 Acceptor::handleRead */
        acceptor_->setNewConnectionCallback(
            std::bind(&TcpServer::newConnection, 
            this, std::placeholders::_1, std::placeholders::_2));
}


TcpServer::~TcpServer() {

}


/* 根据轮询算法选择一个subloop 唤醒subloop 把当前connfd封装为相应的channel 分发给subloop */
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {

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