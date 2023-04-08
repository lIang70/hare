#include <hare/net/host_address.h>
#include "hare/net/socket_op.h"
#include <hare/base/logging.h>

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

        static thread_local char t_resolve_cache[2 * HARE_SMALL_FIXED_SIZE * HARE_SMALL_FIXED_SIZE * HARE_SMALL_FIXED_SIZE];

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

    auto HostAddress::resolve(const std::string& hostname, HostAddress* result) -> bool
    {
        HARE_ASSERT(result != nullptr, "");
        struct hostent host_ent { };
        struct hostent* host_p { nullptr };
        auto herrno { 0 };
        setZero(&host_ent, sizeof(host_ent));

        auto ret = ::gethostbyname_r(hostname.c_str(), &host_ent, detail::t_resolve_cache, sizeof(detail::t_resolve_cache), &host_p, &herrno);
        if (ret == 0 && host_p != nullptr) {
            HARE_ASSERT(host_p->h_addrtype == AF_INET && host_p->h_length == sizeof(uint32_t), "");
            result->d_->addr_in_.sin_addr = *reinterpret_cast<struct in_addr*>(host_p->h_addr);
            return true;
        }

        if (ret != 0) {
            SYS_ERROR() << "::gethostbyname_r error";
        }
        return false;
    }

    auto HostAddress::localAddress(util_socket_t target_fd) -> HostAddress
    {
        HostAddress local_addr {};
        auto addr_len = static_cast<socklen_t>(sizeof(*local_addr.d_));
        if (::getsockname(target_fd, socket::sockaddr_cast(&local_addr.d_->addr_in6_), &addr_len) < 0) {
            SYS_ERROR() << "Cannot get local addr.";
        }
        return local_addr;
    }

    auto HostAddress::peerAddress(util_socket_t target_fd) -> HostAddress
    {
        HostAddress local_addr {};
        auto addr_len = static_cast<socklen_t>(sizeof(*local_addr.d_));
        if (::getpeername(target_fd, socket::sockaddr_cast(&local_addr.d_->addr_in6_), &addr_len) < 0) {
            SYS_ERROR() << "Cannot get peer addr.";
        }
        return local_addr;
    }

    HostAddress::HostAddress(uint16_t port, bool loopback_only, bool ipv6)
        : d_(new Data)
    {
        static_assert(sizeof(HostAddress::Data) == sizeof(struct sockaddr_in6), "HostAddress is same size as sockaddr_in6");
        static_assert(offsetof(HostAddress::Data, addr_in6_) == 0, "addr_in6_ offset 0");
        static_assert(offsetof(HostAddress::Data, addr_in_) == 0, "addr_in_ offset 0");
        if (ipv6) {
            setZero(&(d_->addr_in6_), sizeof(d_->addr_in6_));
            d_->addr_in6_.sin6_family = AF_INET6;
            in6_addr addr = loopback_only ? in6addr_loopback : in6addr_any;
            d_->addr_in6_.sin6_addr = addr;
            d_->addr_in6_.sin6_port = socket::hostToNetwork16(port);
        } else {
            setZero(&(d_->addr_in_), sizeof(d_->addr_in_));
            d_->addr_in_.sin_family = AF_INET;
            in_addr_t addr = loopback_only ? INADDR_ANY : INADDR_LOOPBACK;
            d_->addr_in_.sin_addr.s_addr = socket::hostToNetwork32(addr);
            d_->addr_in_.sin_port = socket::hostToNetwork16(port);
        }
    }

    HostAddress::HostAddress(const std::string& target_ip, uint16_t port, bool ipv6)
        : d_(new Data)
    {
        if (ipv6 || (strchr(target_ip.c_str(), ':') != nullptr)) {
            setZero(&(d_->addr_in6_), sizeof(d_->addr_in6_));
            socket::fromIpPort(target_ip.c_str(), port, &(d_->addr_in6_));
        } else {
            setZero(&(d_->addr_in_), sizeof(d_->addr_in_));
            socket::fromIpPort(target_ip.c_str(), port, &(d_->addr_in_));
        }
    }

    HostAddress::HostAddress(const HostAddress& another) noexcept
        : d_(new Data)
    {
        if (d_ == nullptr) {
            SYS_FATAL() << "Cannot alloc new host address";
        }
        *d_ = *(another.d_);
    }

    HostAddress::~HostAddress()
    {
        delete d_;
    }

    auto HostAddress::operator=(const HostAddress& another) noexcept -> HostAddress&
    {
        if (this == &another) {
            return (*this);
        }
        
        *d_ = *(another.d_);
        return (*this);
    }

    auto HostAddress::getSockAddr() const -> sockaddr*
    {
        return socket::sockaddr_cast(&d_->addr_in6_);
    }

    void HostAddress::setSockAddrInet6(const struct sockaddr_in6* addr_in6)
    {
        HARE_ASSERT(addr_in6 != nullptr, "");
        d_->addr_in6_ = *addr_in6;
    }

    auto HostAddress::toIp() const -> std::string
    {
        char cache[HARE_SMALL_FIXED_SIZE * 2] {};
        socket::toIp(cache, sizeof(cache), socket::sockaddr_cast(&d_->addr_in6_));
        return cache;
    }

    auto HostAddress::toIpPort() const -> std::string
    {
        char cache[HARE_SMALL_FIXED_SIZE * 2] {};
        socket::toIpPort(cache, sizeof(cache), socket::sockaddr_cast(&d_->addr_in6_));
        return cache;
    }

    auto HostAddress::port() const -> uint16_t
    {
        return socket::networkToHost16(portNetEndian());
    }

    auto HostAddress::ipv4NetEndian() const -> uint32_t
    {
        HARE_ASSERT(d_->addr_in_.sin_family == AF_INET, "");
        return d_->addr_in_.sin_addr.s_addr;
    }

    auto HostAddress::portNetEndian() const -> uint16_t
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
