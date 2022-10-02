#include <mymuduo/TcpServer.hh>
#include <mymuduo/Logger.hh>
#include <string>
#include <functional>

/* test Echo Server using mymuduo */

class EchoServer {
public:
    EchoServer(EventLoop* loop, const InetAddress& addr, const std::string& name)
        : server_(loop, addr, name)
        , loop_(loop)
        {
            using namespace std::placeholders;
            server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, _1));
            server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, _1, _2, _3));
            server_.setThreadNum(3); /* 3 subloop */
    }
    void start() { server_.start(); }
private:
    /* 连接建立和关闭事件回调 */
    void onConnection(const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        } else {
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }
    }
    /* 消息回调 */
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time) {
        std::string msg = buffer->retrieveAllAsString();
        conn->send(msg); /* 回显 */
        conn->shutdown();
    }
    EventLoop* loop_;  /* mainloop */
    TcpServer server_;
};


int main() {
    EventLoop loop; /* mainloop */;
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "EchoServer-01");
    server.start();
    loop.loop();    /* start mainloop epoll_wait */
    return 0;
}