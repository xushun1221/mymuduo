#ifndef   __BUFFER_HH_
#define   __BUFFER_HH_

#include <vector>
#include <string>
#include <algorithm>

/*
                                 Buffer
    +--------------------+--------------------+--------------------+
    | prependable bytes  |   readable bytes   |   writable bytes   |
    |                    |     (CONTENT)      |                    | 
    +--------------------+--------------------+--------------------+
    |                    |                    |                    |
    0         <=    readerIndex    <=   writerIndex      <=       size

    Buffer的内存空间由std::vector<char>管理

    prependable: 预留区 最小长度为kCheapPrepend = 8 会随着readerIndex的后移而扩大
    readable:    可读区 长度为writerIndex - readerIndex 会随着writerIndex的后移而扩大
    writable:    可写区 长度为buffer.size() - writerIndex 可以随着数组扩容而扩大 或者因为buffer空间重新调整而扩大
    readerIndex: 可读区首地址 标记可以读取的第一个字符的地址 每读取一个字节 readerIndex后移一个字节
    writerIndex: 可写区首地址 标记可以写入的第一个字符空间的地址 每写入一个字节 writerIndex后移一个字节

    Buffer使用方法: writable区紧跟着readable区 先写再读 写入的数据直接进入可读区
                   可写区写入字符后 writerIndex后移(可读区扩大)
                   写入的字符就直接进入可读区的末尾了 可以被读取了
*/

class Buffer {
public:

    static const size_t kCheapPrepend = 8;      /* 预留区最小长度 */
    static const size_t kInitialSize = 1024;    /* 缓冲区初始大小 */

    explicit Buffer(size_t initialSize = kInitialSize) 
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
        {
    }
    ~Buffer() = default;

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }   /* 可读数据长度 */
    size_t writableBytes() const { return buffer_.size() - writerIndex_; } /* 可写缓冲区长度 */
    size_t prependableBytes() const { return readerIndex_; }               /* 预留区长度(最小8) */

    const char* peek() const { return begin() + readerIndex_; } /* 获得可读数据首地址 */

/* 读数据相关的接口 */
    /* 从缓冲区收回len长度的可读空间 */
    void retrieve(size_t len) {
        if (len < readableBytes()) {
            /* 没有全部读完可读数据 */
            readerIndex_ += len;
        } else { /* len == readableBytes() */
            retrieveAll();
        }
    }
    /* 从缓冲区收回全部空间 */
    void retrieveAll() {  
        /* 全部字符都被读取了 直接重置readerIndex_和writerIndex_ */
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }
    /* 把缓冲区中全部可读字符装载为string 并收回全部可读空间 */
    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes()); 
    }
    /* 从缓冲区中装载长度为len的可读字符为string 并回收这些空间 */
    std::string retrieveAsString(size_t len) {
        std::string result(peek(), len); /* 指定范围的可读字符串转为string */
        retrieve(len); /* 将刚刚(上句)读取出的字符空间在可读区中回收 */
        return result;
    }

/* 写数据相关的接口 */
    /* 向Buffer中写数据前 需要确保Buffer中有足够的空间可写 */
    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) {
            /* 可写空间不足 需要进行空间扩展 */
            makeSpace(len);
        }
    }

    /* 向Buffer可写区中写入data上长度为len的数据 */
    void append(const char* data, size_t len) {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len; /* 写入数据 可写区变小 可读区扩大 */
    }

    /* 获得可写区第一个字节内存地址 */
    char* beginWrite() { return begin() + writerIndex_; }
    const char* beginWrite() const { return begin() + writerIndex_; }


/* 读写文件描述符 */
    /* 从fd读取数据到Buffer */
    ssize_t readFd(int fd, int* savedErrno);
    /* 从Buffer写数据到fd */
    ssize_t writeFd(int fd, int* savedErrno);

private:
    /* 获得Buffer底层数组首地址 对socket进行操作需要字符串指针 从vector底层数组获取 */
    char* begin() { return &*buffer_.begin(); } /* it.operator*()获得char再&获得地址 */
    const char* begin() const { return &*buffer_.begin(); }
    

    /* 对底层数组空间进行扩容或重新调整 满足可写区有len长度的空间 */
    void makeSpace(size_t len) {
        /* 
            如果prepend预留区和可写区总长度足够 就无需扩容
            只需把可读区向前移动即可在数组后面留出足够的空间
            否则必须进行扩容
        */
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
            /* 空间不足 需要扩容 */
            buffer_.resize(writerIndex_ + len); /* 保证writableBytes()有len长度 */
        } else {
            /* 空间可以满足 前移可读区 在数组后面留出足够空间即可 */
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,   /* start */
                      begin() + writerIndex_,   /* end */
                      begin() + kCheapPrepend); /* dst */
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_; /* 缓冲区数组 */
    size_t readerIndex_;       /* 可读起始位置 */
    size_t writerIndex_;        /* 可写起始位置 */

};


#endif // __BUFFER_HH_