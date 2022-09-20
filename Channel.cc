#include "Channel.hh"
#include "EventLoop.hh"
#include "Logger.hh"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;                   /* 没有事件 */
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;  /* 读事件 */
const int Channel::kWriteEvent = EPOLLOUT;           /* 写事件 */

/* 构造和析构 */
Channel::Channel(EventLoop* loop, int fd) 
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false) {}
Channel::~Channel() {

}

/* 防止当Channel被Poller手动remove Channel还在执行回调 */
void Channel::tie(const std::shared_ptr<void>& obj) {
    tie_ = obj; /* 弱智能指针绑定强智能指针 */
    tied_ = true;
}

/* 更新在poller上注册的事件 */
void Channel::update() {
    /* 通过Channel所属的EventLoop 调用Poller的相应方法注册fd的events事件 */
    loop_->updateChannel(this);
}

/* 在Channel所属的EventLoop中 删除当前的Channel */
void Channel::remove() {
    loop_->removeChannel(this);
}

/* 处理事件 调用相应的回调 */
void Channel::handleEvent(Timestamp receiveTime) {
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock(); /* 提升 */
        if (guard) { /* 提升成功 */
            handleEventWithGuard(receiveTime);
        }
    } else {
        handleEventWithGuard(receiveTime);
    }
}

/* 受保护的handleEvent  由handleEvent调用 */
void Channel::handleEventWithGuard(Timestamp receiveTime) {

    LOG_INFO("Channel handleEvent revents: %d", revents_);

    /* 执行相应的回调 */
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        /* 关闭 如果有closeCallback_就执行之*/
        if (closeCallback_) {
            closeCallback_();
        }
    }
    if (revents_ & EPOLLERR) {
        /* 出错 如果有errorCallback_就执行之 */
        if (errorCallback_) {
            errorCallback_();
        }
    }
    if (revents_ & (EPOLLIN | EPOLLPRI)) {
        /* 可读事件 */
        if (readCallback_) {
            readCallback_(receiveTime);
        }
    }
    if (revents_ & EPOLLOUT) {
        /* 可写事件 */
        if (writeCallback_) {
            writeCallback_();
        }
    }
}