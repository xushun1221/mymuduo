#ifndef   __CHANNEL_HH_
#define   __CHANNEL_HH_

#include "noncopyable.hh"
#include "Timestamp.hh"

#include <functional>
#include <memory>

/* 前置声明类类型 而不包含头文件 在.cc文件中包含头文件 可以暴漏更少信息 */
class EventLoop;

/* Channel通道类 封装sockfd及其感兴趣的EPOLL事件以及poller返回的具体事件 */
class Channel : noncopyable {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    /* 构造和析构 使用fd和EventLoop指针构造 */
    explicit Channel(EventLoop* loop, int fd);  /* EventLoop不用包含头文件是因为只需要指针而非具体信息 指针大小都一样 */
    ~Channel();
    
    /* 防止当Channel被Poller手动remove Channel还在执行回调 */
    void tie(const std::shared_ptr<void>& obj);
    
    /* 处理事件 调用相应的回调 */
    void handleEvent(Timestamp receiveTime);    /* Timestamp不用前置类型声明而是包含头文件 因为这里需要知道它的具体大小信息 */
    /* 设置回调函数对象 */
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); } /* cb是局部对象 出作用域就析构 所以使用move右值赋值 转移资源所属权 */
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    /* 返回fd */
    int fd() const { return fd_; }
    /* 返回fd感兴趣的事件 */
    int events() const { return events_; }
    /* 设置poller返回的发生的事件 提供给Poller使用 */
    void set_revents(int revt) { revents_ = revt; }
    
    /* 更新事件 */
    void enableReading() { events_ |= kReadEvent; update(); } /* update epoll_ctl */
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update();}

    /* 当前的事件状态 */
    bool isReading () const { return events_ & kReadEvent; }
    bool isWriting () const { return events_ & kWriteEvent; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    /* 索引号 */
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }
    /* 该Channel所属的EventLoop  one loop per thread */
    EventLoop* ownerLoop() { return loop_; }

    /* 在Channel所属的EventLoop中 删除当前的Channel */
    void remove();

private:
    /* 更新在poller上注册的事件 */
    void update();

    /* 受保护的handleEvent  由handleEvent调用 */
    void handleEventWithGuard(Timestamp receiveTime);

private:
    /* 事件标识 */
    static const int kNoneEvent;  /* 没有事件 */
    static const int kReadEvent;  /* 读事件 */
    static const int kWriteEvent; /* 写事件 */

    EventLoop* loop_;   /* 事件循环 */
    const int fd_;      /* fd poller监听的对象 */
    int events_;        /* 注册fd感兴趣的事件 */
    int revents_;       /* poller返回的具体发生的事件 */
    int index_; /* 表示该Channel在Poller中的状态 -1 1 2 */

    std::weak_ptr<void> tie_;
    bool tied_;

    /* 
        不同事件的回调 
        Channel通道能够获知fd发生的具体的事件revents 所以它负责调用具体事件的回调
    */
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};


#endif // __CHANNEL_HH_