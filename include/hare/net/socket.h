#ifndef _HARE_NET_SOCKET_H_
#define _HARE_NET_SOCKET_H_

#include <hare/base/detail/non_copyable.h>
#include <hare/base/util.h>
#include <hare/net/host_address.h>

namespace hare {
namespace net {

    class HARE_API Socket : public NonCopyable {
        util_socket_t socket_ { -1 };

    public:
        explicit Socket(util_socket_t socket)
            : socket_(socket)
        {
        }

        Socket(Socket&& another) noexcept
        {
            std::swap(socket_, another.socket_);
        }

        ~Socket();

        Socket& operator=(Socket&& another) noexcept
        {
            std::swap(socket_, another.socket_);
            return (*this);
        }

        inline util_socket_t socket() { return socket_; }

        //! abort if address in use
        void bindAddress(const HostAddress& local_addr);
        //! abort if address in use
        void listen();

        //! On success, returns a non-negative integer that is
        //! a descriptor for the accepted socket, which has been
        //! set to non-blocking and close-on-exec. *peeraddr is assigned.
        //! On error, -1 is returned, and *peeraddr is untouched.
        util_socket_t accept(HostAddress& peer_addr);

        void shutdownWrite();

        //!
        //! Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
        //!
        void setTcpNoDelay(bool on);

        //!
        //! Enable/disable SO_REUSEADDR
        //!
        void setReuseAddr(bool on);

        //!
        //! Enable/disable SO_REUSEPORT
        //!
        void setReusePort(bool on);

        //!
        //! Enable/disable SO_KEEPALIVE
        //!
        void setKeepAlive(bool on);
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_SOCKET_H_