#ifndef _HARE_NET_HOST_ADDRESS_H_
#define _HARE_NET_HOST_ADDRESS_H_

#include <hare/base/util/non_copyable.h>
#include <hare/net/util.h>

#include <string>

namespace hare {
namespace net {

    class HARE_API host_address : public util::non_copyable {
        union {
            struct sockaddr_in* in_;
            struct sockaddr_in6* in6_;
        } addr_ {};

    public:
        using ptr = std::shared_ptr<host_address>;

        /**
         * @brief resolve hostname to IP address, not changing port or sin_family.
         *
         *   thread-safe.
         *
         * @return true on success.
         */
        static auto resolve(const std::string& _hostname, host_address* _result) -> bool;
        static auto local_address(util_socket_t _fd) -> host_address;
        static auto peer_address(util_socket_t _fd) -> host_address;

        /**
         * @brief Constructs an endpoint with given port number.
         *   Mostly used in server listening.
         */
        explicit host_address(uint16_t _port = 0, bool _loopback_only = false, bool _ipv6 = false);

        /**
         * @brief Constructs an endpoint with given ip and port.
         * @c ip should be "1.2.3.4"
         */
        host_address(const std::string& _ip, uint16_t _port, bool _ipv6 = false);
        ~host_address();

        host_address(host_address&& _another) noexcept;
        auto operator=(host_address&& _another) noexcept -> host_address&;

        inline auto get_sockaddr() const -> sockaddr* { return sockaddr_cast(addr_.in6_); }
        void set_sockaddr_in6(const struct sockaddr_in6* addr_in6) const;

        auto to_ip() const -> std::string;
        auto to_ip_port() const -> std::string;
        inline auto port() const -> uint16_t { return network_to_host16(port_net_endian()); }

        auto ipv4_net_endian() const -> uint32_t;
        auto port_net_endian() const -> uint16_t;

        // set IPv6 ScopeID
        void set_scope_id(uint32_t _id) const;
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_HOST_ADDRESS_H_