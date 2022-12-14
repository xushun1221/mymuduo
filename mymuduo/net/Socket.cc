#include "Socket.hh"
#include "../base/Logger.hh"
#include "InetAddress.hh"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>


Socket::~Socket() {
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localaddr) {
    if (0 != ::bind(sockfd_, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in))) {
        /* 绑定失败属于严重错误 */
        LOG_FATAL("bind sockfd:%d fail\n", sockfd_);
    }
}

void Socket::listen() {
    if (0 != ::listen(sockfd_, 1024)) {
        /* 设定listen失败属于严重错误 */
        LOG_FATAL("listen sockfd:%d fail\n", sockfd_);
    }
}

int Socket::accept(InetAddress* peeraddr) {
    /* 接受新连接 peeraddr是传出参数 返回fd */
    sockaddr_in addr;
    ::bzero(&addr, sizeof(addr));
    socklen_t len = sizeof(addr); /* 注意 初始化不能省略 */
    /* 这里使用accept4可以直接设置非阻塞 */
    int connfd = ::accept4(sockfd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0) {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

/* 关闭写端 */
void Socket::shutdownWrite() {
    if (::shutdown(sockfd_, SHUT_WR) < 0) {
        /* shutdown出错是一般错误 */
        LOG_ERROR("shutdownWrite error\n");
    }
}


void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof(optval)));
    /* #include <netinet/tcp.h> */
}

void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval)));
}

void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof(optval)));
}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(optval)));
}