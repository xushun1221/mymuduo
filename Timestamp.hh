#ifndef   __TIMESTAMP_HH_
#define   __TIMESTAMP_HH_

#include <iostream>
#include <string>


/* 时间戳类 */
class Timestamp {
public:
    /* 构造 */
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    /* 获得当前时间戳 */
    static Timestamp now();
    /* 时间戳转字符串 */
    std::string toString() const;
private:
    /* 微秒数时间戳 */
    int64_t microSecondsSinceEpoch_;
};


#endif // __TIMESTAMP_HH_