#include "CurrentThread.hh"

#include <sys/syscall.h>

namespace CurrentThread {

    /* 在.cc文件中定义全局变量 __thread修饰表示每个线程中使用独立实体 */

    /* 线程唯一标识 LWP */
    __thread int t_cachedTid = 0;

    /* 将该线程的LWP缓存到线程中 */
    void cacheTid() {
        if (t_cachedTid == 0) {
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }

}