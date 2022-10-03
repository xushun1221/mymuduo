#include "Poller.hh"
#include "Channel.hh"

/* channel是否在当前Poller中 */
bool Poller::hasChannel(Channel* channel) const {
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}



/* 
    如果我们在这里实现newDefaultPoller
    需要返回某个实例化的EpollPoller对象
    那势必需要在这里包含 派生类EpollPoller的头文件
    但是在基类的实现中包含派生类的头文件是不合适的
    所以我们不在这里实现newDeafultPoller
    使用一个独立的源文件DefaultPoller.cc来实现
*/

// #include "EpollPoller.hh"
// Poller* Poller::newDefaultPoller(EventLoop* loop) {
// }
