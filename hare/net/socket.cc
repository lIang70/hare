#include "hare/net/socket_op.h"
#include <hare/base/logging.h>
#include <hare/net/socket.h>

#ifdef HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
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

} // namespace net
} // namespace hare
