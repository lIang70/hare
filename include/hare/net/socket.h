/**
 * @file hare/net/socket.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with socket.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_SOCKET_H_
#define _HARE_NET_SOCKET_H_

#include <hare/base/util/non_copyable.h>
#include <hare/net/error.h>
#include <hare/net/host_address.h>

namespace hare {
namespace net {

    enum Type {
        TYPE_INVALID,
        TYPE_TCP
    };

#if !defined(HARE_NET_TYPE)
#define HARE_NET_TYPE           \
    {                           \
        "invalid", "tcp"        \
    }
#endif

    static constexpr const char* type_name[] HARE_NET_TYPE;

    HARE_INLINE
    constexpr const char* TypeToStr(Type _type)
    {
        return type_name[_type];
    }

    HARE_CLASS_API
    class HARE_API Socket : public util::NonCopyable {
        util_socket_t socket_ { -1 };
        std::uint8_t family_ { 0 };
        Type type_ { TYPE_INVALID };

    public:
        Socket(std::uint8_t _family, Type _type, util_socket_t _socket = -1);
        ~Socket();

        HARE_INLINE auto fd() const -> util_socket_t { return socket_; }
        HARE_INLINE auto family() const -> std::uint8_t { return family_; }
        HARE_INLINE auto type() const -> Type { return type_; }

        auto BindAddress(const HostAddress& _local_addr) const -> Error;
        auto Listen() const -> Error;
        auto Close() -> Error;

        /**
         * @brief On success, returns a non-negative integer that is
         *   a descriptor for the accepted socket, which has been
         *   set to non-blocking and close-on-exec. peeraddr is assigned.
         *
         *   On error, -1 is returned, and peeraddr is untouched.
         *
         */
        auto Accept(HostAddress& _peer_addr) const -> util_socket_t;

        auto ShutdownWrite() const -> Error;

        /**
         * @brief Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
         *
         */
        auto SetTcpNoDelay(bool _no_delay) const -> Error;

        /**
         * @brief Enable/disable SO_REUSEADDR
         *
         */
        auto SetReuseAddr(bool _reuse) const -> Error;

        /**
         *  @brief Enable/disable SO_REUSEPORT
         *
         */
        auto SetReusePort(bool _reuse) const -> Error;

        /**
         *  @brief Enable/disable SO_KEEPALIVE
         *
         */
        auto SetKeepAlive(bool _keep_alive) const -> Error;
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_SOCKET_H_