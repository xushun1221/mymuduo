#ifndef   __EVENTLOOPTHREAD_HH_
#define   __EVENTLOOPTHREAD_HH_

#include "noncopyable.hh"
#include "Thread.hh"

#include <functional>
#include <mutex>
#include <condition_variable>

class EventLoop;

/* one loop per thread! */
class EventLoopThread : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback()
                    , const std::string& name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();
private:
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_; /* 在线程中创建新EventLoop后调用该函数 */
};


#endif // __EVENTLOOPTHREAD_HH_