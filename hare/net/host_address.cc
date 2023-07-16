#include "hare/base/fwd-inl.h"
#include "hare/net/socket_op.h"
#include <hare/net/host_address.h>

#include <cassert>
#include <cstring>

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif // HARE__HAVE_NETINET_IN_H

#if HARE__HAVE_NETDB_H
#include <netdb.h>
#endif // HARE__HAVE_NETDB_H

namespace hare {
namespace net {

    namespace detail {

        static thread_local std::array<char, static_cast<size_t>(2) * HARE_SMALL_FIXED_SIZE * HARE_SMALL_FIXED_SIZE * HARE_SMALL_FIXED_SIZE> t_resolve_cache {};

    } // namespace detail

    static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
    static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
    static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
    static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

    auto host_address::resolve(const std::string& _hostname, host_address* _result) -> bool
    {
        assert(_result != nullptr);
        struct hostent host_ent { };
        struct hostent* host_p { nullptr };
        auto herrno { 0 };

        hare::detail::fill_n(&host_ent, sizeof(host_ent), 0);

        auto ret = ::gethostbyname_r(_hostname.c_str(), &host_ent, detail::t_resolve_cache.data(), detail::t_resolve_cache.size(), &host_p, &herrno);
        if (ret == 0 && host_p != nullptr) {
            assert(host_p->h_addrtype == AF_INET && host_p->h_length == sizeof(uint32_t));
            _result->addr_.in_->sin_addr = *reinterpret_cast<struct in_addr*>(host_p->h_addr);
            return true;
        }

        if (ret != 0) {
            MSG_ERROR("::gethostbyname_r error.");
        }
        return false;
    }

    auto host_address::local_address(util_socket_t _fd) -> host_address
    {
        host_address local_addr {};
        local_addr.addr_.in6_ = static_cast<struct sockaddr_in6*>(malloc(sizeof(struct sockaddr_in6)));

        auto addr_len = static_cast<socklen_t>(sizeof(struct sockaddr_in6));
        if (::getsockname(_fd, sockaddr_cast(local_addr.addr_.in6_), &addr_len) < 0) {
            MSG_ERROR("cannot get local addr.");
        }
        return local_addr;
    }

    auto host_address::peer_address(util_socket_t _fd) -> host_address
    {
        host_address peer_addr {};
        peer_addr.addr_.in6_ = static_cast<struct sockaddr_in6*>(malloc(sizeof(struct sockaddr_in6)));

        auto addr_len = static_cast<socklen_t>(sizeof(struct sockaddr_in6));
        if (::getpeername(_fd, sockaddr_cast(peer_addr.addr_.in6_), &addr_len) < 0) {
            MSG_ERROR("cannot get peer addr.");
        }
        return peer_addr;
    }

    host_address::host_address(uint16_t _port, bool _loopback_only, bool _ipv6)
    {
        static_assert(offsetof(host_address, addr_.in6_) == 0, "addr_in6_ offset 0");
        static_assert(offsetof(host_address, addr_.in_) == 0, "addr_in_ offset 0");

        addr_.in6_ = static_cast<struct sockaddr_in6*>(malloc(sizeof(struct sockaddr_in6)));

        if (_ipv6) {
            hare::detail::fill_n((addr_.in6_), sizeof(struct sockaddr_in6), 0);
            addr_.in6_->sin6_family = AF_INET6;
            auto addr = _loopback_only ? in6addr_loopback : in6addr_any;
            addr_.in6_->sin6_addr = addr;
            addr_.in6_->sin6_port = host_to_network16(_port);
        } else {
            hare::detail::fill_n((addr_.in_), sizeof(struct sockaddr_in), 0);
            addr_.in_->sin_family = AF_INET;
            auto addr = _loopback_only ? INADDR_LOOPBACK : INADDR_ANY;
            addr_.in_->sin_addr.s_addr = addr;
            addr_.in_->sin_port = host_to_network16(_port);
        }
    }

    host_address::host_address(const std::string& _ip, uint16_t _port, bool _ipv6)
    {
        addr_.in6_ = static_cast<struct sockaddr_in6*>(malloc(sizeof(struct sockaddr_in6)));

        if (_ipv6 || (strchr(_ip.c_str(), ':') != nullptr)) {
            hare::detail::fill_n(addr_.in6_, sizeof(struct sockaddr_in6), 0);
            socket_op::from_ip_port(_ip.c_str(), _port, addr_.in6_);
        } else {
            hare::detail::fill_n(addr_.in_, sizeof(struct sockaddr_in), 0);
            socket_op::from_ip_port(_ip.c_str(), _port, addr_.in_);
        }
    }

    host_address::~host_address()
    {
        free(addr_.in6_);
    }

    host_address::host_address(host_address&& _another) noexcept
    {
        std::swap(addr_.in6_, _another.addr_.in6_);
    }

    auto host_address::operator=(host_address&& _another) noexcept -> host_address&
    {
        std::swap(addr_.in6_, _another.addr_.in6_);
        return (*this);
    }

    void host_address::set_sockaddr_in6(const struct sockaddr_in6* _addr_in6) const
    {
        assert(_addr_in6 != nullptr);
        assert(addr_.in6_ != nullptr);
        ::memcpy(addr_.in6_, _addr_in6, sizeof(struct sockaddr_in6));
    }

    auto host_address::to_ip() const -> std::string
    {
        std::array<char, static_cast<size_t>(HARE_SMALL_FIXED_SIZE) * 2> cache {};
        socket_op::to_ip(cache.data(), static_cast<size_t>(HARE_SMALL_FIXED_SIZE) * 2, sockaddr_cast(addr_.in6_));
        return cache.data();
    }

    auto host_address::to_ip_port() const -> std::string
    {
        std::array<char, static_cast<size_t>(HARE_SMALL_FIXED_SIZE) * 2> cache {};
        socket_op::to_ip_port(cache.data(), static_cast<size_t>(HARE_SMALL_FIXED_SIZE) * 2, sockaddr_cast(addr_.in6_));
        return cache.data();
    }

    auto host_address::ipv4_net_endian() const -> uint32_t
    {
        assert(addr_.in_->sin_family == AF_INET);
        return addr_.in_->sin_addr.s_addr;
    }

    auto host_address::port_net_endian() const -> uint16_t
    {
        return addr_.in_->sin_port;
    }

    // set IPv6 ScopeID
    void host_address::set_scope_id(uint32_t _id) const
    {
        if (addr_.in_->sin_family == AF_INET6) {
            addr_.in6_->sin6_scope_id = _id;
        }
    }

} // namespace net
} // namespace hare
