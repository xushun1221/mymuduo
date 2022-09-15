#include "Logger.hh"
#include "Timestamp.hh"

#include <iostream>

/* 获取Logger唯一的实例对象 */
Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

/* 设置日志级别 */
void Logger::setLogLevel(int level){
    logLevel_ = level;
}

/* 写日志接口 */
void Logger::log(std::string msg){
    /* 日志格式： [级别信息] time : msg */
    switch (logLevel_) {
        case INFO: 
            std::cout << "[INFO]";
            break;
        case ERROR: 
            std::cout << "[ERROR]";
            break;
        case FATAL: 
            std::cout << "[FATAL]";
            break;
        case DEBUG: 
            std::cout << "[DEBUG]";
            break;
    }
    /* 打印time和msg */
    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}
