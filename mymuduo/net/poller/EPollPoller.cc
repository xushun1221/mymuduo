#include "EPollPoller.hh"
#include "../../base/Logger.hh"
#include "../Channel.hh"

#include <errno.h>
#include <string.h>
#include <unistd.h>

/* Channel::index_为下面三种状态之一 */
const int kNew = -1;    /* Channel没添加到EPollPoller */
const int kAdded = 1;   /* Channel已添加到EPollPoller */
const int kDelete = 2;  /* Channel已从EPollPoller中删除 只是从监听树上取下 仍然在哈希表中 */


/* 构造和析构 */
EPollPoller::EPollPoller(EventLoop* loop) 
    : Poller(loop) 
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC)) /* 创建一个epollfd */
    , events_(kInitEventListSize) /* events_初始大小16 */
    { 
        if (epollfd_ < 0) {
            LOG_FATAL("epoll_create1 error: %d \n", errno);
        }
}
EPollPoller::~EPollPoller() {
    ::close(epollfd_);
}

/* 更新Channel在Poller中的状态 使用私有方法update() */
void EPollPoller::updateChannel(Channel* channel) {
    /* 获得Channel在Poller中的状态 */
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);
    /* 没添加到监听树或从监听树上取下的Channel */
    if (index == kNew || index == kDelete) {
        if (index == kNew) {
            /* 新添加到监听树上的Channel */
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        /* 更新状态 */
        channel->set_index(kAdded);
        /* 使用epoll_ctl函数的EPOLL_CTL_ADD操作将Channel添加到监听树 */
        update(EPOLL_CTL_ADD, channel);
    } else {
        /* 还在监听树上的Channel */
        if (channel->isNoneEvent()) {
            /* 如果该Channel没有要监听的事件 就把他从监听树上取下 */
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDelete);
            /* 
                注意 如果Channel上没有要监听的事件 把他从监听树上取下 
                但并不会从哈希表中删除 如果以后有要监听的事件会再上树 
            */
        } else {
            /* 监听的事件改变了 就修改要监听的事件 */
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

/* 把Channel从Poller中删除 */
void EPollPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    channels_.erase(fd);
 
    LOG_INFO("func=%s => fd=%d \n", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded) {
        /* 如果Channel还在监听树上 把它取下 */
        update(EPOLL_CTL_DEL, channel);
    }
    /* 如果已经从监听树上取下(kDelete) 无需再取下 */
    channel->set_index(kNew);
}

/* epoll_ctl add/mod/del */
void EPollPoller::update(int operation, Channel* channel) {
    epoll_event event;
    bzero(&event, sizeof(event));
    int fd = channel->fd();
    event.events = channel->events();
    // event.data.fd = fd;
    /* 
        注意 epoll_event.data中fd和ptr不能同时使用 
        会相互覆盖掉的 
        妈的这个把我坑惨了
    */
    event.data.ptr = static_cast<void*>(channel); /* 携带了一个参数 */
    /* 调用epoll_ctl执行operation对应的操作 */
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            /* 如果EPOLL_CTL_DEL出错 可以认为是ERROR级别 程序依然可以运行 */
            LOG_ERROR("epoll_ctl del error: %d\n", errno);
        } else {
            /* 如果EPOLL_CTL_ADD或EPOLL_CTL_MOD出错 程序无法继续运行 认为是FATAL级别错误 */
            LOG_FATAL("epoll_ctl add/mod error: %d\n", errno);
        }
    }
}


/* epoll_wait的封装 将发生事件的Channel通过activeChannels参数告知EventLoop */
Timestamp EPollPoller::poll(int timeoutMs, Poller::ChannelList* activeChannels) {
    /* 应该用LOG_DEBUG更合理些 */
    LOG_INFO("func=%s => fd total count:%zd\n", __FUNCTION__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_, 
                                 &*events_.begin(), /* 用成员变量events_接收发生的监听事件 */
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
    /* errno是全局的变量 多个线程的poll都有可能访问它 所以先存起来 */
    int saveErrno = errno;

    /* 记录当前时间 */
    Timestamp now(Timestamp::now());

    if (numEvents > 0) {
        /* 有发生事件的fd */
        LOG_INFO("func=%s => %d events happened\n", __FUNCTION__, numEvents);

        /* 将发生事件的Channel填到activeChannels中 传给EventLoop进行处理 */
        fillActiaveChannels(numEvents, activeChannels);

        if (static_cast<size_t>(numEvents) == events_.size()) {
            /* 发生事件的fd数量和events_的容量相同了 此时需要扩容了 */
            events_.resize(events_.size() * 2);
        }
    } else if(numEvents == 0) {
        /* 没有监听事件发生 */
        LOG_DEBUG("func=%s => timeout \n", __FUNCTION__);
    } else { /* numEvents < 0 */
        /* 如果错误是EINTR外部中断 无需处理 */
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR("func=%s => epoll_wait error\n", __FUNCTION__);
        }
    }
    return now;
}

/* 将发生监听事件的Channel填入activeChannels中 以便EventLoop进行处理 */
void EPollPoller::fillActiaveChannels(int numEvents, Poller::ChannelList* activeChannels) const {
    /* activeChannels实参是由EventLoop提供的 */
    for (int i = 0; i < numEvents; ++ i) {
        /* 获得发生事件的fd对应的Channel */
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        /* 将发生的事件告知Channel */
        channel->set_revents(events_[i].events);
        /* 填入发生事件列表 */
        activeChannels->push_back(channel);
    }
    /* EventLoop就拿到了发生监听事件的Channel列表 进行处理 */
}
