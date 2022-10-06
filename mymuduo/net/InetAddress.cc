#include "InetAddress.hh"

#include <string.h>

/* 使用ip+port构造socket地址 点分十进制*/
InetAddress::InetAddress(uint16_t port, std::string ip) {
    /* 置零 string.h */
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    int dst;
    inet_pton(AF_INET, ip.c_str(), (void*)&dst);
    addr_.sin_addr.s_addr = dst;
}

/* 获取ip字符串 点分十进制*/
std::string InetAddress::toIp() const {
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr.s_addr, buf, sizeof(buf));
    return buf;
}

/* 获取ip+port字符串 xxx.xxx.xxx.xxx:xxxx*/
std::string InetAddress::toIpPort() const {
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr.s_addr, buf, sizeof(buf));
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + strlen(buf), ":%u", port);
    return buf;
}

/* 获取port */
uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}
