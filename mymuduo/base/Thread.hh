#ifndef   __THREAD_HH_
#define   __THREAD_HH_

#include "noncopyable.hh"

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

/* Thread线程类 记录一个新线程的详细信息 */
class Thread : noncopyable {
public:
    /* 线程函数类型 结合bind绑定器使用 */
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    pid_t itd() const { return tid_; }
    const std::string& name() const { return name_; }

    static int32_t numCreated() { return numCreated_; }
private:
    void setDefaultName();

    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;

    static std::atomic_int32_t numCreated_;
};



#endif // __THREAD_HH_