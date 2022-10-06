#include "../Poller.hh"
#include "EPollPoller.hh"

#include <stdlib.h>

/* EventLoop可以通过该接口获得默认IO复用的具体实现 */
Poller* Poller::newDefaultPoller(EventLoop* loop) {
    /* 默认提供epoll的实例 如果环境变量中有MUDUO_USE_POLL则返回poll的实例 */
    if (::getenv("MUDUO_USE_POLL")) {
        return nullptr; // add code 生成poll的实例
    } else {
        return new EPollPoller(loop);
    }
}
