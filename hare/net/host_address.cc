#include "hare/net/socket_op.h"
#include <hare/base/logging.h>
#include <hare/net/host_address.h>

#include <cstddef>

#ifdef HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif // HARE__HAVE_NETINET_IN_H

#ifdef HARE__HAVE_NETDB_H
#include <netdb.h>
#endif // HARE__HAVE_NETDB_H

namespace hare {
namespace net {

    namespace detail {

        static thread_local char t_resolve_cache[64 * 1024];

    } // namespace detail

    class HostAddress::Data {
    public:
        union {
            struct sockaddr_in addr_in_;
            struct sockaddr_in6 addr_in6_;
        };
    };

    static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
    static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
    static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
    static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");
    static_assert(sizeof(HostAddress::Data) == sizeof(struct sockaddr_in6), "InetAddress is same size as sockaddr_in6");

    bool HostAddress::resolve(const std::string& hostname, HostAddress* result)
    {
        HARE_ASSERT(result != nullptr, "");
        struct hostent host_ent { };
        struct hostent* host_p { nullptr };
        auto herrno { 0 };
        setZero(&host_ent, sizeof(host_ent));

        int ret = ::gethostbyname_r(hostname.c_str(), &host_ent, detail::t_resolve_cache, sizeof(detail::t_resolve_cache), &host_p, &herrno);
        if (ret == 0 && host_p != NULL) {
            HARE_ASSERT(host_p->h_addrtype == AF_INET && host_p->h_length == sizeof(uint32_t), "");
            result->d_->addr_in_.sin_addr = *reinterpret_cast<struct in_addr*>(host_p->h_addr);
            return true;
        } else {
            if (ret) {
                SYS_ERROR() << "::gethostbyname_r error";
            }
            return false;
        }
    }

    HostAddress::HostAddress(uint16_t port, bool loopback_only, bool ipv6)
        : d_(new Data)
    {
        static_assert(offsetof(HostAddress::Data, addr_in6_) == 0, "addr_in6_ offset 0");
        static_assert(offsetof(HostAddress::Data, addr_in_) == 0, "addr_in_ offset 0");
        if (ipv6) {
            setZero(&(d_->addr_in6_), sizeof(d_->addr_in6_));
            d_->addr_in6_.sin6_family = AF_INET6;
            in6_addr ip = loopback_only ? in6addr_loopback : in6addr_any;
            d_->addr_in6_.sin6_addr = ip;
            d_->addr_in6_.sin6_port = socket::hostToNetwork16(port);
        } else {
            setZero(&(d_->addr_in_), sizeof(d_->addr_in_));
            d_->addr_in_.sin_family = AF_INET;
            in_addr_t ip = loopback_only ? INADDR_ANY : INADDR_LOOPBACK;
            d_->addr_in_.sin_addr.s_addr = socket::hostToNetwork32(ip);
            d_->addr_in_.sin_port = socket::hostToNetwork16(port);
        }
    }

    HostAddress::HostAddress(const std::string& ip, uint16_t port, bool ipv6)
        : d_(new Data)
    {
        if (ipv6 || strchr(ip.c_str(), ':')) {
            setZero(&(d_->addr_in6_), sizeof(d_->addr_in6_));
            socket::fromIpPort(ip.c_str(), port, &(d_->addr_in6_));
        } else {
            setZero(&(d_->addr_in_), sizeof(d_->addr_in_));
            socket::fromIpPort(ip.c_str(), port, &(d_->addr_in_));
        }
    }

    HostAddress::HostAddress(const HostAddress& another)
        : d_(new Data)
    {
        *d_ = *another.d_;
    }

    HostAddress::~HostAddress()
    {
        delete d_;
    }

    HostAddress& HostAddress::operator=(const HostAddress& another)
    {
        *d_ = *another.d_;
        return (*this);
    }

    sockaddr* HostAddress::getSockAddr() const
    {
        return socket::sockaddr_cast(&d_->addr_in6_);
    }

    void HostAddress::setSockAddrInet6(const struct sockaddr_in6* addr_in6)
    {
        HARE_ASSERT(addr_in6 != nullptr, "");
        d_->addr_in6_ = *addr_in6;
    }

    std::string HostAddress::toIp() const
    {
        char cache[64] {};
        socket::toIp(cache, sizeof(cache), socket::sockaddr_cast(&d_->addr_in6_));
        return cache;
    }

    std::string HostAddress::toIpPort() const
    {
        char cache[64] {};
        socket::toIpPort(cache, sizeof(cache), socket::sockaddr_cast(&d_->addr_in6_));
        return cache;
    }

    uint16_t HostAddress::port() const
    {
        return socket::networkToHost16(portNetEndian());
    }

    uint32_t HostAddress::ipv4NetEndian() const
    {
        HARE_ASSERT(d_->addr_in_.sin_family == AF_INET, "");
        return d_->addr_in_.sin_addr.s_addr;
    }

    uint16_t HostAddress::portNetEndian() const
    {
        return d_->addr_in_.sin_port;
    }

    // set IPv6 ScopeID
    void HostAddress::setScopeId(uint32_t scope_id)
    {
        if (d_->addr_in_.sin_family == AF_INET6) {
            d_->addr_in6_.sin6_scope_id = scope_id;
        }
    }

} // namespace net
} // namespace hare
