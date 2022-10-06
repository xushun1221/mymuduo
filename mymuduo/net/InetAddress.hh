#ifndef   __INETADDRESS_HH_
#define   __INETADDRESS_HH_

#include <arpa/inet.h>
#include <string>

/* socket地址类型封装 */
class InetAddress {
public:
    /* 使用ip+port构造socket地址 */
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    /* 使用sockaddr_in直接构造socket地址 */
    explicit InetAddress(const sockaddr_in& addr) : addr_(addr) {}
    /* 获取ip字符串 */
    std::string toIp() const;
    /* 获取ip+port字符串 */
    std::string toIpPort() const;
    /* 获取port */
    uint16_t toPort() const;
    /* 获得内部的sockaddr_in */
    const sockaddr_in* getSockAddr() const { return &addr_; }
    /* 使用sockaddr_in设置InetAddress */
    void setSockAddr(const sockaddr_in& addr) { addr_ = addr;}

private:
    /* socket地址类型 仅支持ipv4 */
    sockaddr_in addr_;
};


#endif // __INETADDRESS_HH_