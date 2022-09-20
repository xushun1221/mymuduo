#ifndef   __EVENTLOOP_HH_
#define   __EVENTLOOP_HH_

#include "noncopyable.hh"
#include "Timestamp.hh"
#include "CurrentThread.hh"

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

/* EventLoop事件循环类 主要包含Channel和Poller(epoll)两大模块 */
class EventLoop : noncopyable {
public:
    /* 回调的类型 */
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    /* 开启事件循环 */
    void loop();
    /* 退出事件循环 */
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    /* 在当前loop中执行cb */
    void runInLoop(Functor cb);
    /* 把cb放入队列中 唤醒loop所在线程执行cb */
    void queueInLoop(Functor cb);

    /* mainloop唤醒subloop 唤醒loop所在线程 */
    void wakeup();

    /* 更新Channel状态 调用Poller的方法*/
    void updateChannel(Channel* channel);
    /* 删除Channel 调用Poller的方法 */
    void removeChannel(Channel* channel);
    /* 判断Channel是否存在 调用Poller的方法 */
    bool hasChannel(Channel* channel);

    /* EventLoop是否在当前线程 */
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:

    /* wakeup相关 */
    void handleRead();
    /* 执行回调 */
    void doPendingFunctors();


    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; /* 标识是否正在looping */
    std::atomic_bool quit_;    /* 控制是否跳出loop */

    const pid_t threadId_; /* 当前loop所在线程的id(LWP) */  

    Timestamp pollReturnTime_; /* Poller返回发生事件的Channel的时间戳 */
    std::unique_ptr<Poller> poller_; /* EventLoop管理的Poller */

    /* 重要!!!!!!!!! */
    int wakeupFd_; 
    std::unique_ptr<Channel> wakeupChannel_; /* 用于封装wakeupFd_ */
        /* 
            当mainloop获取一个新用户的Channel时
            通过轮询算法选择一个subloop
            通过该成员唤醒subloop处理channel
            使用eventfd()系统调用函数来实现该功能
        */
    /* !!!!!!!!!!!!! */

    ChannelList activeChannels_; /* 发生事件的Channel列表 */

    std::atomic_bool callingPendingFunctors_; /* 标识当前loop是否有需要执行的回调 */
    std::vector<Functor> pendingFunctors_; /* 存放loop需要执行的所有的回调操作 */
    std::mutex mutex_; /* 用来保护pendingFunctors_ */
};


#endif // __EVENTLOOP_HH_