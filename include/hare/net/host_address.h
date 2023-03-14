#ifndef _HARE_NET_HOST_ADDRESS_H_
#define _HARE_NET_HOST_ADDRESS_H_

#include <hare/base/util/non_copyable.h>
#include <hare/base/util/util.h>

#include <cinttypes>
#include <string>

struct sockaddr;
struct sockaddr_in6;

namespace hare {
namespace net {

    class HARE_API HostAddress {
        class Data;
        Data* d_ { nullptr };

    public:
        using Ptr = std::shared_ptr<HostAddress>;

        //! resolve hostname to IP address, not changing port or sin_family
        //! return true on success.
        //! thread safe
        static bool resolve(const std::string& hostname, HostAddress* result);

        static HostAddress localAddress(util_socket_t fd);
        static HostAddress peerAddress(util_socket_t fd);

        //! Constructs an endpoint with given port number.
        //! Mostly used in TcpServer listening.
        explicit HostAddress(uint16_t port = 0, bool loopback_only = false, bool ipv6 = false);

        //! Constructs an endpoint with given ip and port.
        //! @c ip should be "1.2.3.4"
        HostAddress(const std::string& ip, uint16_t port, bool ipv6 = false);
        HostAddress(const HostAddress& another) noexcept;
        HostAddress(HostAddress&& another) noexcept
        {
            std::swap(d_, another.d_);
        }
        ~HostAddress();

        HostAddress& operator=(const HostAddress& another) noexcept;
        HostAddress& operator=(HostAddress&& another) noexcept
        {
            std::swap(d_, another.d_);
            return (*this);
        }

        sockaddr* getSockAddr() const;
        void setSockAddrInet6(const struct sockaddr_in6* addr_in6);

        std::string toIp() const;
        std::string toIpPort() const;
        uint16_t port() const;

        uint32_t ipv4NetEndian() const;
        uint16_t portNetEndian() const;

        // set IPv6 ScopeID
        void setScopeId(uint32_t scope_id);
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_HOST_ADDRESS_H_