#include "EventLoopThreadPool.hh"
#include "EventLoopThread.hh"


EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop, const std::string& nameArg) 
    : baseloop_(baseloop) /* 注意baseloop_是传入的 */
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0)
    {
}

EventLoopThreadPool::~EventLoopThreadPool() {
    /* 无需释放loops_ 因为线程绑定的loop都是栈上对象 */
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb = ThreadInitCallback()) {
    started_ = true;
    /* 服务器开启多个线程 */
    for (int i = 0; i < numThreads_; ++ i) {
        /* 线程名 = 线程池名 + 序号 */
        char buf[name_.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
        /* 创建EventLoopThread并加入线程池 */
        EventLoopThread* t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        /* 启动线程 绑定一个EventLoop 并获得线程对应绑定的loop地址 */
        loops_.push_back(t->startLoop());
    }

    /* 服务器只有一个线程 baseloop(mainloop) */
    if (numThreads_ == 0 && cb) {
        cb(baseloop_);
    }
}

/* 如果工作在多线程中 baseloop_会默认以轮询方式分配Channel给subloop */
EventLoop* EventLoopThreadPool::getNextLoop() {
    /* 如果只使用了一个线程baseloop 返回baseloop */
    EventLoop* loop = baseloop_;
    if (loops_.empty() == false) {
        /* 如果使用了多个线程 轮询获得下一个处理事件的loop */
        loop = loops_[next_];
        ++ next_;
        if (next_ >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops() {
    if (loops_.empty()) { 
        /* 如果仅有baseloop */
        return std::vector<EventLoop*>(1, baseloop_);
    } else {
        return loops_;
    }
}