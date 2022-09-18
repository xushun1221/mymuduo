#ifndef   __LOGGER_HH_
#define   __LOGGER_HH_

#include <string>
#include <stdlib.h>

#include "noncopyable.hh"

/* 定义四个宏 对应使用四个级别的日志 */

/* LOG_INFO("%s %d", arg1, arg2) */
#define LOG_INFO(logmsgFormat, ...) \
    do { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)
/*
    snprintf将格式化的字符串拷贝到buf中
    ##__VA_ARGS__表示可变参数 如果可变参数为空 它会自动去掉前面的逗号
*/

#define LOG_ERROR(logmsgFormat, ...) \
    do { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)

/* 如果严重错误 执行exit(-1) 退出程序 */
#define LOG_FATAL(logmsgFormat, ...) \
    do { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while(0)

/* 
    对于调试信息 我们仅在调试时进行使用 正常使用时输出大量调试信息会降低效率
    当指定了MUDEBUG时输出调试信息 未指定时不输出调试信息(使用空宏)
*/
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do { \
        Logger& logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0)
#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif


/* 定义日志的级别 */
enum LogLevel {
    INFO,   // 普通信息
    ERROR,  // 错误信息
    FATAL,  // core信息
    DEBUG   // 调试信息
};


/* 日志类 Logger 单例模式 不可拷贝构造及赋值*/
class Logger : noncopyable {
public:
    /* 获取Logger唯一的实例对象 */
    static Logger& instance();
    /* 设置日志级别 */
    void setLogLevel(int level);
    /* 写日志接口 */
    void log(std::string msg);
private:
    Logger() {}
    int logLevel_;
};


#endif // __LOGGER_HH_