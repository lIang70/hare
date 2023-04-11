#include "hare/base/error.h"
#include "hare/base/util.h"
#include "hare/net/socket_op.h"
#include <cstddef>
#include <hare/base/logging.h>
#include <hare/net/socket.h>
#include <sys/socket.h>

#ifdef HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HARE__HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

namespace hare {
namespace net {

    Socket::Socket(int8_t family, TYPE type, util_socket_t socket)
        : socket_(socket)
        , family_(family)
        , type_(type)
    {
        if (socket == -1) {
            switch (type) {
            case TYPE_TCP:
                socket_ = socket::createNonblockingOrDie(family);
                break;
            case TYPE_UDP:
                socket_ = socket::createDgramOrDie(family);
                break;
            case TYPE_INVALID:
            default:
                LOG_FATAL() << "Unrecognized type of socket.";
            }
        }
    }

    Socket::~Socket()
    {
        close();
    }

    auto Socket::bindAddress(const HostAddress& local_addr) const -> Error
    {
        return socket::bind(socket_, local_addr.getSockAddr());
    }

    auto Socket::listen() const -> Error
    {
        return socket::listen(socket_);
    }

    auto Socket::close() -> Error
    {
        if (socket_ != -1) {
            auto err = socket::close(socket_);
            socket_ = -1;
            return err;
        }
        return Error(HARE_ERROR_SUCCESS);
    }

    auto Socket::accept(HostAddress& peer_addr, HostAddress* local_addr) const -> util_socket_t
    {
        struct sockaddr_in6 addr { };
        util_socket_t accept_fd { -1 };
        setZero(&addr, sizeof(addr));
        if (type_ == TYPE_TCP) {
            accept_fd = socket::accept(socket_, &addr);
        } else {
            if (local_addr == nullptr) {
                return accept_fd;
            }
            size_t addr_len = family_ == PF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            socket::recvfrom(socket_, nullptr, 0, sockaddrCast(&addr), addr_len);
            accept_fd = socket::createDgramOrDie(family_);
            auto reuse { 1 };
            auto ret = setsockopt(accept_fd , SOL_SOCKET, SO_REUSEADDR, &reuse,sizeof(reuse));
            if (ret < 0) {
                socket::close(accept_fd);
                return -1;
            }
            ret = setsockopt(accept_fd , SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
            if (ret < 0) {
                socket::close(accept_fd);
                return -1;
            }
            auto err = socket::bind(accept_fd, local_addr->getSockAddr());
            if (!err) {
                socket::close(accept_fd);
                return -1;
            }
            err = socket::connect(accept_fd, sockaddrCast(&addr));
            if (!err) {
                socket::close(accept_fd);
                return -1;
            }
        }
        if (accept_fd >= 0) {
            peer_addr.setSockAddrInet6(&addr);
        }
        return accept_fd;
    }

    void Socket::shutdownWrite() const
    {
        socket::shutdownWrite(socket_);
    }

    auto Socket::setTcpNoDelay(bool no_delay) const -> Error
    {
        auto opt_val = no_delay ? 1 : 0;
        auto ret = ::setsockopt(socket_, SOL_SOCKET, TCP_NODELAY, &opt_val, static_cast<socklen_t>(sizeof(opt_val)));
        return ret < 0 ? Error(HARE_ERROR_SOCKET_TCP_NO_DELAY) : Error(HARE_ERROR_SUCCESS);
    }

    auto Socket::setReuseAddr(bool reuse) const -> Error
    {
        auto optval = reuse ? 1 : 0;
        auto ret = ::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval)));
        return ret < 0 ? Error(HARE_ERROR_SOCKET_REUSE_ADDR) : Error(HARE_ERROR_SUCCESS);
    }

    auto Socket::setReusePort(bool reuse) const -> Error
    {
#ifdef SO_REUSEPORT
        auto opt_val = reuse ? 1 : 0;
        auto ret = ::setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &opt_val, static_cast<socklen_t>(sizeof(opt_val)));
#else
        SYS_ERROR() << "Reuse-port is not supported.";
        auto ret = -1;
#endif
        return ret < 0 ? Error(HARE_ERROR_SOCKET_REUSE_PORT) : Error(HARE_ERROR_SUCCESS);
    }

    auto Socket::setKeepAlive(bool keep_alive) const -> Error
    {
        auto optval = keep_alive ? 1 : 0;
        auto ret = ::setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(optval)));
        return ret < 0 ? Error(HARE_ERROR_SOCKET_KEEP_ALIVE) : Error(HARE_ERROR_SUCCESS);
    }

} // namespace net
} // namespace hare
