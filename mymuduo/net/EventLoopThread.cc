#include "EventLoopThread.hh"
#include "EventLoop.hh"


EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name) 
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name) /* 注意这里线程还没创建 线程调用start()后才创建 */
    , mutex_()
    , cond_()
    , callback_(cb)
    {   
}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop() {
    /* 启动新线程 */
    thread_.start(); /* 线程执行的函数就是下面的EventLoopThread::threadFunc() */

    /* 注意 这里是多线程环境了 */

    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr) {
            cond_.wait(lock); /* 等待新线程初始化loop_ */
        }
        /* 初始化loop_完成 */
        loop = loop_;
    }
    return loop; /* 返回新线程中的loop对象 */
}

/* 该方法是在新线程中运行的 */
void EventLoopThread::threadFunc() {
    /* 创建一个独立的EventLoop 和新线程是一一对应的 */
    EventLoop loop; /* one loop per thread */

    if (callback_) {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); /* 开启poller.poll */

    /* 事件循环结束 */
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}