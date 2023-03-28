#include "hare/net/socket_op.h"
#include <asm-generic/socket.h>
#include <hare/base/logging.h>
#include <hare/net/socket.h>

#ifdef HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HARE__HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

namespace hare {
namespace net {

    Socket::Socket(int8_t family, util_socket_t socket)
        : socket_(socket)
    {
        if (socket == -1) {
            switch (family) {
            case AF_INET:
            case AF_INET6:
                socket = socket::createNonblockingOrDie(family);
                break;
            default:
                LOG_FATAL() << "Unrecognized family.";
            }
        }
    }

    Socket::~Socket()
    {
        if (socket_ != -1) {
            socket::close(socket_);
        }
    }

    auto Socket::bindAddress(const HostAddress& local_addr) const -> bool
    {
        return socket::bind(socket_, local_addr.getSockAddr());
    }

    auto Socket::listen() const -> bool
    {
        return socket::listen(socket_);
    }

    void Socket::close()
    {
        socket::close(socket_);
        socket_ = -1;
    }

    auto Socket::accept(HostAddress& peer_addr) const -> util_socket_t
    {
        struct sockaddr_in6 addr {};
        setZero(&addr, sizeof(addr));
        auto accept_fd = socket::accept(socket_, &addr);
        if (accept_fd >= 0) {
            peer_addr.setSockAddrInet6(&addr);
        }
        return accept_fd;
    }

    void Socket::shutdownWrite() const
    {
        socket::shutdownWrite(socket_);
    }

    void Socket::setTcpNoDelay(bool no_delay) const
    {
        auto opt_val = no_delay ? 1 : 0;
        auto ret = ::setsockopt(socket_, SOL_SOCKET, TCP_NODELAY, &opt_val, static_cast<socklen_t>(sizeof(opt_val)));
        if (ret < 0) {
            LOG_ERROR() << "Fail to set tcp no-delay.";
        }
    }

    void Socket::setReuseAddr(bool reuse) const
    {
        auto optval = reuse ? 1 : 0;
        auto ret = ::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval)));
        if (ret < 0) {
            LOG_ERROR() << "Fail to set reuse address.";
        }
    }

    void Socket::setReusePort(bool reuse) const
    {
#ifdef SO_REUSEPORT
        auto opt_val = reuse ? 1 : 0;
        auto ret = ::setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &opt_val, static_cast<socklen_t>(sizeof(opt_val)));
        if (ret < 0) {
            LOG_ERROR() << "Fail to set tcp reuse port.";
        }
#else
        LOG_ERROR() << "Reuse-port is not supported.";
#endif
    }

    void Socket::setKeepAlive(bool keep_alive) const
    {
        auto optval = keep_alive ? 1 : 0;
        auto ret = ::setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(optval)));
        if (ret < 0) {
            LOG_ERROR() << "Fail to set tcp keep alive.";
        }
    }

} // namespace net
} // namespace hare
