#include "hare/net/socket_op.h"
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

    Socket::~Socket()
    {
        if (socket_ != -1) {
            socket::close(socket_);
        }
    }

    void Socket::bindAddress(const HostAddress& local_addr)
    {
        socket::bindOrDie(socket_, local_addr.getSockAddr());
    }

    void Socket::listen()
    {
        socket::listenOrDie(socket_);
    }

    util_socket_t Socket::accept(HostAddress& peer_addr)
    {
        struct sockaddr_in6 addr;
        setZero(&addr, sizeof(addr));
        auto fd = socket::accept(socket_, &addr);
        if (fd >= 0) {
            peer_addr.setSockAddrInet6(&addr);
        }
        return fd;
    }

    void Socket::shutdownWrite()
    {
        socket::shutdownWrite(socket_);
    }

    void Socket::setTcpNoDelay(bool on)
    {
        auto optval = on ? 1 : 0;
        auto ret = ::setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof(optval)));
        if (ret < 0) {
            LOG_ERROR() << "Fail to set tcp no-delay.";
        }
    }

    void Socket::setReuseAddr(bool on)
    {
        auto optval = on ? 1 : 0;
        auto ret = ::setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof(optval)));
        if (ret < 0) {
            LOG_ERROR() << "Fail to set tcp no-delay.";
        }
    }

    void Socket::setReusePort(bool on)
    {
#ifdef SO_REUSEPORT
        auto optval = on ? 1 : 0;
        auto ret = ::setsockopt(socket_, IPPROTO_TCP, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval)));
        if (ret < 0) {
            LOG_ERROR() << "Fail to set tcp no-delay.";
        }
#else
        LOG_ERROR() << "Reuse-port is not supported.";
#endif
    }

    void Socket::setKeepAlive(bool on)
    {
        auto optval = on ? 1 : 0;
        auto ret = ::setsockopt(socket_, IPPROTO_TCP, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(optval)));
        if (ret < 0) {
            LOG_ERROR() << "Fail to set tcp no-delay.";
        }
    }

} // namespace net
} // namespace hare
