#ifndef _HARE_NET_SOCKET_H_
#define _HARE_NET_SOCKET_H_

#include <hare/base/error.h>
#include <hare/base/util/non_copyable.h>
#include <hare/net/host_address.h>

namespace hare {
namespace net {

    using TYPE = enum Type {
        TYPE_INVALID,
        TYPE_TCP,
        TYPE_UDP
    };

    class HARE_API socket : public non_copyable {
        util_socket_t socket_ { -1 };
        int8_t family_ { 0 };
        TYPE type_ { TYPE_INVALID };

    public:
        using ptr = std::shared_ptr<socket>;

        static auto type_str(TYPE _type) -> const char*;

        socket(int8_t family, TYPE type, util_socket_t socket = -1);
        ~socket();

        inline auto fd() const -> util_socket_t { return socket_; }
        inline auto family() const -> int8_t { return family_; }
        inline auto type() const -> TYPE { return type_; }

        auto bind_address(const host_address& local_addr) const -> error;
        auto listen() const -> error;
        auto close() -> error;

        /**
         * @brief On success, returns a non-negative integer that is
         *   a descriptor for the accepted socket, which has been
         *   set to non-blocking and close-on-exec. peeraddr is assigned.
         *
         *   On error, -1 is returned, and peeraddr is untouched.
         *
         */
        auto accept(host_address& peer_addr) const -> util_socket_t;

        auto shutdown_write() const -> error;

        /**
         * @brief Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
         *
         */
        auto set_tcp_no_delay(bool no_delay) const -> error;

        /**
         * @brief Enable/disable SO_REUSEADDR
         *
         */
        auto set_reuse_addr(bool reuse) const -> error;

        //!
        //! Enable/disable SO_REUSEPORT
        //!
        auto set_reuse_port(bool reuse) const -> error;

        //!
        //! Enable/disable SO_KEEPALIVE
        //!
        auto set_keep_alive(bool keep_alive) const -> error;
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_SOCKET_H_