#include <hare/net/host_address.h>

#include "hare/net/socket_op.h"
#include <hare/base/logging.h>

#include <cstddef>
#include <cstring>

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
            result->addr_.in_->sin_addr = *reinterpret_cast<struct in_addr*>(host_p->h_addr);
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
        local_addr.addr_.in6_ = static_cast<struct sockaddr_in6*>(malloc(sizeof(struct sockaddr_in6)));

        auto addr_len = static_cast<socklen_t>(sizeof(struct sockaddr_in6));
        if (::getsockname(target_fd, sockaddrCast(local_addr.addr_.in6_), &addr_len) < 0) {
            SYS_ERROR() << "Cannot get local addr.";
        }
        return local_addr;
    }

    auto HostAddress::peerAddress(util_socket_t target_fd) -> HostAddress
    {
        HostAddress peer_addr {};
        peer_addr.addr_.in6_ = static_cast<struct sockaddr_in6*>(malloc(sizeof(struct sockaddr_in6)));

        auto addr_len = static_cast<socklen_t>(sizeof(struct sockaddr_in6));
        if (::getpeername(target_fd, sockaddrCast(peer_addr.addr_.in6_), &addr_len) < 0) {
            SYS_ERROR() << "Cannot get peer addr.";
        }
        return peer_addr;
    }

    HostAddress::HostAddress(uint16_t port, bool loopback_only, bool ipv6)
    {
        static_assert(offsetof(HostAddress, addr_.in6_) == 0, "addr_in6_ offset 0");
        static_assert(offsetof(HostAddress, addr_.in_) == 0, "addr_in_ offset 0");

        addr_.in6_ = static_cast<struct sockaddr_in6*>(malloc(sizeof(struct sockaddr_in6)));

        if (ipv6) {
            setZero((addr_.in6_), sizeof(struct sockaddr_in6));
            addr_.in6_->sin6_family = AF_INET6;
            in6_addr addr = loopback_only ? in6addr_loopback : in6addr_any;
            addr_.in6_->sin6_addr = addr;
            addr_.in6_->sin6_port = hostToNetwork16(port);
        } else {
            setZero((addr_.in_), sizeof(struct sockaddr_in));
            addr_.in_->sin_family = AF_INET;
            in_addr_t addr = loopback_only ? INADDR_ANY : INADDR_LOOPBACK;
            addr_.in_->sin_addr.s_addr = hostToNetwork32(addr);
            addr_.in_->sin_port = hostToNetwork16(port);
        }
    }

    HostAddress::HostAddress(const std::string& target_ip, uint16_t port, bool ipv6)
    {
        addr_.in6_ = static_cast<struct sockaddr_in6*>(malloc(sizeof(struct sockaddr_in6)));

        if (ipv6 || (strchr(target_ip.c_str(), ':') != nullptr)) {
            setZero(addr_.in6_, sizeof(struct sockaddr_in6));
            socket::fromIpPort(target_ip.c_str(), port, addr_.in6_);
        } else {
            setZero(addr_.in_, sizeof(struct sockaddr_in));
            socket::fromIpPort(target_ip.c_str(), port, addr_.in_);
        }
    }

    HostAddress::~HostAddress()
    {
        free(addr_.in6_);
    }

    HostAddress::HostAddress(HostAddress&& another) noexcept
    {
        std::swap( addr_.in6_, another.addr_.in6_);
    }

    auto HostAddress::operator=(HostAddress&& another) noexcept -> HostAddress&
    {
        std::swap( addr_.in6_, another.addr_.in6_);
        return (*this);
    }

    void HostAddress::setSockAddrInet6(const struct sockaddr_in6* addr_in6) const
    {
        HARE_ASSERT(addr_in6 != nullptr, "");
        HARE_ASSERT(addr_.in6_ != nullptr, "");
        memcpy(addr_.in6_, addr_in6, sizeof(struct sockaddr_in6));
    }

    auto HostAddress::toIp() const -> std::string
    {
        char cache[HARE_SMALL_FIXED_SIZE * 2] {};
        socket::toIp(cache, sizeof(cache), sockaddrCast(addr_.in6_));
        return cache;
    }

    auto HostAddress::toIpPort() const -> std::string
    {
        char cache[HARE_SMALL_FIXED_SIZE * 2] {};
        socket::toIpPort(cache, sizeof(cache), sockaddrCast(addr_.in6_));
        return cache;
    }

    auto HostAddress::ipv4NetEndian() const -> uint32_t
    {
        HARE_ASSERT(addr_.in_->sin_family == AF_INET, "");
        return addr_.in_->sin_addr.s_addr;
    }

    auto HostAddress::portNetEndian() const -> uint16_t
    {
        return addr_.in_->sin_port;
    }

    // set IPv6 ScopeID
    void HostAddress::setScopeId(uint32_t scope_id)
    {
        if (addr_.in_->sin_family == AF_INET6) {
            addr_.in6_->sin6_scope_id = scope_id;
        }
    }

} // namespace net
} // namespace hare
