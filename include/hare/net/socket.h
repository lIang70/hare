#ifndef _HARE_NET_SOCKET_H_
#define _HARE_NET_SOCKET_H_

#include <cstdint>
#include <hare/base/error.h>
#include <hare/base/util/non_copyable.h>
#include <hare/net/host_address.h>

namespace hare {
namespace net {

    class HARE_API Socket : public NonCopyable {
    public:
        using TYPE = enum Type {
            TYPE_INVALID,
            TYPE_TCP,
            TYPE_UDP
        };

    private:
        util_socket_t socket_ { -1 };
        int8_t family_ { 0 };
        TYPE type_ { TYPE_INVALID };

    public:
        using Ptr = std::shared_ptr<Socket>;

        Socket(int8_t family, TYPE type, util_socket_t socket = -1);
        ~Socket();

        inline auto socket() const -> util_socket_t { return socket_; }
        inline auto family() const -> int8_t { return family_; }
        inline auto type() const -> TYPE { return type_; }

        auto bindAddress(const HostAddress& local_addr) const -> Error;
        auto listen() const -> Error;
        auto close() -> Error;

        /**
         * @brief On success, returns a non-negative integer that is
         *   a descriptor for the accepted socket, which has been
         *   set to non-blocking and close-on-exec. peeraddr is assigned.
         *
         *   On error, -1 is returned, and peeraddr is untouched.
         *
         */
        auto accept(HostAddress& peer_addr, HostAddress* local_addr = nullptr) const -> util_socket_t;

        auto shutdownWrite() const -> Error;

        /**
         * @brief Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
         *
         */
        auto setTcpNoDelay(bool no_delay) const -> Error;

        /**
         * @brief Enable/disable SO_REUSEADDR
         *
         */
        auto setReuseAddr(bool reuse) const -> Error;

        //!
        //! Enable/disable SO_REUSEPORT
        //!
        auto setReusePort(bool reuse) const -> Error;

        //!
        //! Enable/disable SO_KEEPALIVE
        //!
        auto setKeepAlive(bool keep_alive) const -> Error;
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_SOCKET_H_