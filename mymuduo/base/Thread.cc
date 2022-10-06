#include "Thread.hh"
#include "CurrentThread.hh"

#include <semaphore.h>

/* atomic_int32_t禁止使用拷贝构造 不能使用= */
std::atomic_int32_t Thread::numCreated_{0};

Thread::Thread(ThreadFunc func, const std::string& name) 
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
    {
        setDefaultName();
}

Thread::~Thread() {
    if (started_ && !joined_) {
        thread_->detach(); /* 设置线程分离 无需join */
    }
}

void Thread::start() {
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    /* 开启线程 */
    thread_ = std::shared_ptr<std::thread>(
        new std::thread([&](){
            /* 获取线程id */
            tid_ = CurrentThread::tid();
            /* 获取完tid后可以通知start()返回 */
            sem_post(&sem);
            /* 新线程执行的函数 */
            func_();
        })
    );
    /* 必须等新线程获取完线程id才能return */
    sem_wait(&sem);
}

void Thread::join() {
    joined_ = true;
    thread_->join();
}

/* 线程的默认名字使用线程创建的序号 */
void Thread::setDefaultName() {
    int num = ++ numCreated_;
    if (name_.empty()) {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}