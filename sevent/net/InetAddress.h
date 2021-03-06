#ifndef SEVENT_NET_INETADDRESS_H
#define SEVENT_NET_INETADDRESS_H

#include "sevent/net/util.h"
#ifndef _WIN32
#include <netinet/in.h>
#else
#include <ws2tcpip.h>
#endif
#include <string>
namespace sevent {
namespace net {

class InetAddress {
public:
    InetAddress();
    explicit InetAddress(uint16_t port, bool ipv6 = false, bool loopback = false);
    // ip = "1.2.3.4"
    InetAddress(uint16_t port, const std::string& ip, bool ipv6 = false);
    InetAddress(uint16_t port, const char* ip, bool ipv6 = false);
    InetAddress(const std::string& ip, uint16_t port, bool ipv6 = false);
    InetAddress(const char* ip, uint16_t port, bool ipv6 = false);
    explicit InetAddress(const struct sockaddr_in& addr) : addr(addr){}
    explicit InetAddress(const struct sockaddr_in6& addr) : addr6(addr){}

    const struct sockaddr *getSockAddr() const {
        return reinterpret_cast<const struct sockaddr *>(&addr6);
    }
    struct sockaddr *getSockAddr() {
        return reinterpret_cast<struct sockaddr *>(&addr6);
    }
    void setsockAddr(const struct sockaddr_in6 &address) { addr6 = address; }

    std::string toStringIp() const;
    std::string toStringIpPort() const;
    uint16_t getPortHost() const;
    uint32_t getIpNet() const { return addr.sin_addr.s_addr; }
    uint16_t family() const { return addr.sin_family; }

    static bool resolve(const std::string& hostname, InetAddress *iaddr);

private:
    union {
        struct sockaddr_in addr;
        struct sockaddr_in6 addr6;
    };
};

} // namespace net
} // namespace sevent

#endif