#ifndef   __POLLER_HH_
#define   __POLLER_HH_

#include "noncopyable.hh"
#include "Timestamp.hh"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

/* Poller抽象类 多路事件分发器的核心IO复用模块 */
class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop) : ownerLoop_(loop) {}
    virtual ~Poller() = default;

    /* 为所有IO复用方法 提供统一的接口 */
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0; /* epoll_wait */
    virtual void updateChannel(Channel* channel) = 0;   /* epoll_ctl */
    virtual void removeChannel(Channel* Channel) = 0;   /* epoll_ctl */
    
    /* channel是否在当前Poller中 */
    virtual bool hasChannel(Channel* channel) const;

    /* EventLoop可以通过该接口获得默认IO复用的具体实现 */
    static Poller* newDefaultPoller(EventLoop* loop); /* 我们并不在Poller.cc中提供该方法的实现 */
protected:
    /* key为sockfd value为sockfd所属的Channel */
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;  /* 该Poller持有的所有Channel */
private:
    EventLoop* ownerLoop_; /* 该Poller所属的EventLoop */
};


#endif // __POLLER_HH_