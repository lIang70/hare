#ifndef _HARE_NET_SOCKET_H_
#define _HARE_NET_SOCKET_H_

#include <hare/base/util/non_copyable.h>
#include <hare/base/util/util.h>
#include <hare/net/host_address.h>

namespace hare {
namespace net {

    class HARE_API Socket {
        util_socket_t socket_ { -1 };

    public:
        using Ptr = std::shared_ptr<Socket>;

        explicit Socket(int8_t family, util_socket_t socket = -1);

        ~Socket();

        inline auto socket() const -> util_socket_t { return socket_; }

        auto bindAddress(const HostAddress& local_addr) const -> bool;
        auto listen() const -> bool;
        void close();

        //! On success, returns a non-negative integer that is
        //! a descriptor for the accepted socket, which has been
        //! set to non-blocking and close-on-exec. *peeraddr is assigned.
        //! On error, -1 is returned, and *peeraddr is untouched.
        auto accept(HostAddress& peer_addr) const -> util_socket_t;

        void shutdownWrite() const;

        //!
        //! Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
        //!
        void setTcpNoDelay(bool no_delay) const;

        //!
        //! Enable/disable SO_REUSEADDR
        //!
        void setReuseAddr(bool reuse) const;

        //!
        //! Enable/disable SO_REUSEPORT
        //!
        void setReusePort(bool reuse) const;

        //!
        //! Enable/disable SO_KEEPALIVE
        //!
        void setKeepAlive(bool keep_alive) const;
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_SOCKET_H_