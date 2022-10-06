#include "EventLoop.hh"
#include "../base/Logger.hh"
#include "Poller.hh"
#include "Channel.hh"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

/* 防止一个线程创建多个EventLoop 一个线程中只能有一个EventLoop */
__thread EventLoop* t_loopInThisThread = nullptr;

/* 定义默认的Poller的IO复用接口的超时事件 */
const int kPollTimeMs = 10000;

/* 创建wakeupfd 用来唤醒subReactor处理新来的channel */
int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        /* 出错 无法创建evtfd 这是严重错误 */
        LOG_FATAL("eventfd error:%d\n", errno);
    }
    return evtfd;
}

/* 构造 */
EventLoop::EventLoop() 
    : looping_(false)
    , quit_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
    , callingPendingFunctors_(false)
    {
        LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
        if (t_loopInThisThread != nullptr) {
            /* 如果该线程中已经存在了EventLoop无法运行 属于严重错误 */
            LOG_FATAL("another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
        } else {
            /* 该线程第一次创建一个EventLoop对象 */
            t_loopInThisThread = this;
        }
        /* 设置wakeupfd的事件类型以及发生事件后的回调 */
        wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
        /* 每个EventLoop都会监听wakeupChannel的EPOLLIN事件 */
        wakeupChannel_->enableReading();
}

/* 析构 */
EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

/* 接收wakeupFd_收到的唤醒消息 */
void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::handleRead() reads %zu bytes instead of 8 \n", n);
    }
}


/* 开启事件循环 */
void EventLoop::loop() {
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (quit_ == false) {
        activeChannels_.clear();
        /* 获得发生事件的Channel */
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        /* 遍历所有发生事件的Channel 执行对应的回调 */
        for (Channel* channel : activeChannels_) {
            channel->handleEvent(pollReturnTime_);
        }
        /* 执行当前EventLoop事件循环需要处理的回调操作 */
        doPendingFunctors();
        /*
            IO线程 mainloop 主要做accept的工作 fd->Channel => subloop
            1. 如果我们的服务器只使用一个线程 就是mainloop
                那么它不仅要接收新用户的连接 还要处理已连接用户的读写事件
            2. 如果使用了多个线程
                mainloop获得了新用户的Channel后 就会唤醒某一个subloop
                mainloop事先注册一个回调cb 需要subloop来执行
                mainloop wakeup一个subloop时 通过wakeupfd唤醒subloop
                subloop会从poller_->poll的阻塞状态醒来
                然后就会通过doPendingFunctors来执行mainloop之前注册的回调cb
        */
    }
    LOG_INFO("EventLoop %p stop looping \n", this);
    looping_ = false;
}

/* 退出事件循环 */
void EventLoop::quit() {
    quit_ = true;
    /*
        1. 在自己的线程中调用quit()时
            说明没有阻塞在poller_->poll处
        2. 如果在其他线程中调用了该EventLoop的quit()
            需要把该loop唤醒
            再次进入循环时就会发现quit_ == true
    */
    if (isInLoopThread() == false) {
        wakeup();
    }
}


/* 在当前loop中执行cb */
void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread() == true) {
        cb();
    } else {
        /* 在非loop线程中执行cb 就需要唤醒loop所在线程执行cb */
        queueInLoop(cb);
    }

}

/* 把cb放入队列中 唤醒loop所在线程执行cb */
void EventLoop::queueInLoop(Functor cb) {
    /* cb放入pendingFunctors */
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb); /* push_back也可 */
    }
    /* 唤醒相应的需要执行cb的loop的线程 */
    if (isInLoopThread() == false || callingPendingFunctors_ == true) { 
        /* 
            callingPendingFunctors_ == true 表示上一轮的doPendingFunctors正在执行
            执行完进入下一轮循环时 仍然有可能再次阻塞
            所以需要进行唤醒操作
        */
        wakeup(); /* 唤醒该loop所在线程 */
    }
}



/* mainloop唤醒该subloop 唤醒该loop所在线程 */
void EventLoop::wakeup() {
    /* 向该EventLoop的wakeupfd_写一个数据即可唤醒 */
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        /* 无法唤醒该EventLoop */
        LOG_ERROR("EventLoop::wakeup() writes %zu bytes instead of 8 \n", n);
    }
}

/* 更新Channel状态 调用Poller的方法*/
void EventLoop::updateChannel(Channel* channel) {
    poller_->updateChannel(channel);
}

/* 删除Channel 调用Poller的方法 */
void EventLoop::removeChannel(Channel* channel) {
    poller_->removeChannel(channel);
}

/* 判断Channel是否存在 调用Poller的方法 */
bool EventLoop::hasChannel(Channel* channel) {
    return poller_->hasChannel(channel);
}

/* 执行回调 */
void EventLoop::doPendingFunctors() {
    /* 执行mainloop注册到该loop中的回调操作 */

    /*
        注意 这里我们并不能直接对pendingFunctors_进行加锁并遍历执行
        而是需要创建一个局部vector将pendingFunctors_置换出来 再遍历执行
        这是因为
            doPendingFunctors()函数执行可能需要较长时间
            在执行过程中 如果mainloop又注册了新的回调 就会阻塞在互斥锁上
            运行效率很低
        这样做 即使该loop正在执行需要做的回调
        也不妨碍mainloop继续向该loop注册新的回调
    */
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for (const Functor& functor : functors) {
        functor();
    }
    callingPendingFunctors_ = false;
}