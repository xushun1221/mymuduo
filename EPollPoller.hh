#ifndef   __EPOLLPOLLER_HH_
#define   __EPOLLPOLLER_HH_

#include "Poller.hh"
#include "Timestamp.hh"

#include <sys/epoll.h>
#include <vector>

class Channel;

class EPollPoller : public Poller {
public:
    /* 构造和析构 */
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override; /* override确认这是一个覆盖 */

    /* epoll方法接口 */
    Timestamp poll(int timeoutMs, Poller::ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override; /* epoll_ctl add */
    void removeChannel(Channel* channel) override; /* epoll_ctl del */
private:
    static const int kInitEventListSize = 16;  /* EventList初始化大小 */

    /* 将发生监听事件的Channel填入activeChannels中 以便EventLoop进行处理 */
    void fillActiaveChannels(int numEvents, Poller::ChannelList* activeChannels) const;
    /* epoll_ctl add/mod/del */
    void update(int operation, Channel* channel); /* epoll_ctl */

    using EventList = std::vector<epoll_event>;

    int epollfd_;       /* epoll文件系统的描述符 */
    EventList events_;  /* 用于接收发生事件的epoll_event 用作epoll_wait的传出参数 */
};


#endif // __EPOLLPOLLER_HH_