#ifndef   __NONCOPYABLE_HH_
#define   __NONCOPYABLE_HH_

/* 接口类 禁用拷贝构造和赋值 */
class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

#endif // __NONCOPYABLE_HH_