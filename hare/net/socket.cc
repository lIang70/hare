#include "hare/base/fwd-inl.h"
#include "hare/net/socket_op.h"
#include <hare/base/exception.h>
#include <hare/net/socket.h>

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HARE__HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

namespace hare {
namespace net {

    auto socket::type_str(TYPE _type) -> const char*
    {
        switch (_type) {
        case TYPE_TCP:
            return "TCP";
        case TYPE_UDP:
            return "UDP";
        default:
            return "INVALID";
        }
    }

    socket::socket(int8_t family, TYPE type, util_socket_t socket)
        : socket_(socket)
        , family_(family)
        , type_(type)
    {
        if (socket == -1) {
            switch (type) {
            case TYPE_TCP:
                socket_ = socket_op::create_nonblocking_or_die(family);
                break;
            case TYPE_UDP:
                socket_ = socket_op::create_dgram_or_die(family);
                break;
            case TYPE_INVALID:
            default:
                MSG_FATAL("unrecognized type of socket.");
            }
        }
    }

    socket::~socket()
    {
        close();
    }

    auto socket::bind_address(const host_address& local_addr) const -> error
    {
        return socket_op::bind(socket_, local_addr.get_sockaddr());
    }

    auto socket::listen() const -> error
    {
        return socket_op::listen(socket_);
    }

    auto socket::close() -> error
    {
        if (socket_ != -1) {
            auto err = socket_op::close(socket_);
            socket_ = -1;
            return err;
        }
        return error();
    }

    auto socket::accept(host_address& peer_addr) const -> util_socket_t
    {
        struct sockaddr_in6 addr { };
        hare::detail::fill_n(&addr, sizeof(addr), 0);
        auto accept_fd = socket_op::accept(socket_, &addr);
        if (accept_fd >= 0) {
            peer_addr.set_sockaddr_in6(&addr);
        }
        return accept_fd;
    }

    auto socket::shutdown_write() const -> error
    {
        return socket_op::shutdown_write(socket_);
    }

    auto socket::set_tcp_no_delay(bool no_delay) const -> error
    {
        auto opt_val = no_delay ? 1 : 0;
        auto ret = ::setsockopt(socket_, SOL_SOCKET, TCP_NODELAY, &opt_val, static_cast<socklen_t>(sizeof(opt_val)));
        return ret != 0 ? error(ERROR_SOCKET_TCP_NO_DELAY) : error();
    }

    auto socket::set_reuse_addr(bool reuse) const -> error
    {
        auto optval = reuse ? 1 : 0;
        auto ret = ::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval)));
        return ret != 0 ? error(ERROR_SOCKET_REUSE_ADDR) : error();
    }

    auto socket::set_reuse_port(bool reuse) const -> error
    {
#ifdef SO_REUSEPORT
        auto opt_val = reuse ? 1 : 0;
        auto ret = ::setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &opt_val, static_cast<socklen_t>(sizeof(opt_val)));
#else
        SYS_ERROR() << "reuse-port is not supported.";
        auto ret = -1;
#endif
        return ret != 0 ? error(ERROR_SOCKET_REUSE_PORT) : error();
    }

    auto socket::set_keep_alive(bool keep_alive) const -> error
    {
        auto optval = keep_alive ? 1 : 0;
        auto ret = ::setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(optval)));
        return ret != 0 ? error(ERROR_SOCKET_KEEP_ALIVE) : error();
    }

} // namespace net
} // namespace hare
