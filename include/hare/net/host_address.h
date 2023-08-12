/**
 * @file hare/net/host_address.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with host_address.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_HOST_ADDRESS_H_
#define _HARE_NET_HOST_ADDRESS_H_

#include <hare/base/io/socket_op.h>
#include <hare/base/util/non_copyable.h>

struct sockaddr;

namespace hare {
namespace net {

    HARE_CLASS_API
    class HARE_API HostAddress : public util::NonCopyable {
        sockaddr* in_ {};

    public:
        /**
         * @brief resolve hostname to IP address, not changing port or sin_family.
         *
         *   thread-safe.
         *
         * @return true on success.
         */
        static auto Resolve(const std::string& _hostname, HostAddress* _result) -> bool;
        static auto LocalAddress(util_socket_t _fd) -> HostAddress;
        static auto PeerAddress(util_socket_t _fd) -> HostAddress;

        /**
         * @brief Constructs an endpoint with given port number.
         *   Mostly used in server listening.
         */
        explicit HostAddress(std::uint16_t _port = 0, bool _loopback_only = false, bool _ipv6 = false);

        /**
         * @brief Constructs an endpoint with given ip and port.
         * @c ip should be "1.2.3.4"
         */
        HostAddress(const std::string& _ip, std::uint16_t _port, bool _ipv6 = false);
        ~HostAddress();

        HostAddress(HostAddress&& _another) noexcept;
        auto operator=(HostAddress&& _another) noexcept -> HostAddress&;

        auto Clone() const -> hare::Ptr<HostAddress>;

        auto Family() const -> std::uint8_t;

        HARE_INLINE auto get_sockaddr() const -> sockaddr* { return in_; }
        void set_sockaddr_in6(const struct sockaddr_in6* addr_in6) const;

        auto ToIp() const -> std::string;
        auto ToIpPort() const -> std::string;
        HARE_INLINE auto Port() const -> std::uint16_t { return socket_op::NetworkToHost16(PortNetEndian()); }

        auto Ipv4NetEndian() const -> std::uint32_t;
        auto PortNetEndian() const -> std::uint16_t;

        // set IPv6 ScopeID
        void SetScopeId(std::uint32_t _id) const;
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_HOST_ADDRESS_H_