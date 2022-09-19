#ifndef   __CURRENTTHREAD_HH_
#define   __CURRENTTHREAD_HH_

#include <unistd.h>

namespace CurrentThread {
    
    /* 该线程的LWP的缓存 */
    extern __thread int t_cachedTid;

    /* 将该线程的LWP缓存到线程中 */
    void cacheTid();

    /* 获得当前线程的LWP */
    inline int tid() {
        /* 
            __builtin_expect是gcc提供的分支预测优化的宏
            __builtin_expect(exp, c)表示期望exp的值为c
            编译过程中编译器会将可能性更大的代码紧跟前面的代码
            减少指令跳转 优化程序性能

            这里表示t_cachedTid == 0(还没有缓存LWP)的概率很小
            已经缓存的概率很大
            如果没缓存过 调用cacheTid缓存该线程的LWP
        */
        if (__builtin_expect(t_cachedTid == 0, 0)) {
            cacheTid();
        }
        return t_cachedTid;
    }

}


#endif // __CURRENTTHREAD_HH_