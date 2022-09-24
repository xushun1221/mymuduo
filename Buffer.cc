#include "Buffer.hh"

#include <sys/uio.h>
#include <errno.h>



const size_t Buffer::kCheapPrepend; /* 静态常量要在类外定义 */
const size_t Buffer::kInitialSize;


/* 从fd读取数据到Buffer */
ssize_t Buffer::readFd(int fd, int* savedErrno) {
/*
    这里存在一个问题: 
        使用read()或readv()从fd读取数据时 是直接拷贝到char*内存空间上
        如果Buffer的可写空间不够 就无法直接从内核fd缓冲区拷贝到Buffer可写区中
    解决方法:
        使用另一块栈上的extrabuf空间 配合使用readv()读取内核fd缓冲区内容
        如果Buffer中可写空间足够 直接完全写入Buffer中
        如果不够 剩余的内容写入extrabuf空间 然后使用Buffer::append()方法添加到Buffer中
*/
    char extrabuf[65536] = {0}; /* 64K栈上内存 效率高 函数结束随着栈帧回退自动释放 */
    const size_t writable = writableBytes(); /* Buffer可写空间大小 */
    struct iovec vec[2];
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    /* 如果Buffer空间大于64K则不启用extrabuf (一次最多读64K) */
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt); /* 读数据 */
    if (n < 0) {
        *savedErrno = errno; /* 传出错误号 */
    } else if (static_cast<size_t>(n) <= writable) {
        /* 读到的数据可以完全写入Buffer */
        writerIndex_ += n;
    } else { /* n > writable */
        /* Buffer已经写满 且extrabuf中也有数据 */
        writerIndex_ = buffer_.size();
        /* 将extrabuf中数据写入Buffer (Buffer进行了扩容) */
        append(extrabuf, n - writable);
    }
    /* 返回读取的字节数 */
    return n; 
}
