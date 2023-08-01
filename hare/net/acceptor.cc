#include "hare/base/fwd-inl.h"
#include "hare/base/io/reactor.h"
#include <hare/base/io/socket_op.h>
#include <hare/net/acceptor.h>
#include <hare/net/socket.h>
#include <hare/hare-config.h>

#if HARE__HAVE_FCNTL_H
#include <fcntl.h>
#endif // HARE__HAVE_FCNTL_H

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif // HARE__HAVE_NETINET_IN_H

#if defined(H_OS_WIN)
#include <WinSock2.h>
#endif

namespace hare {
namespace net {

    HARE_IMPL_DEFAULT(acceptor,
        socket socket_;
        acceptor::new_session new_session_ {};
        std::uint16_t port_ {};
        acceptor_impl(std::uint8_t _family, TYPE _type, util_socket_t _fd, std::uint16_t _port)
            : socket_(_family, _type, _fd)
            , port_(_port)
        {}
    )

    acceptor::acceptor(std::uint8_t _family, TYPE _type, std::uint16_t _port, bool _reuse_port)
        : io::event(_type == TYPE_TCP ? socket_op::create_nonblocking_or_die(_family) : (_type == TYPE_UDP ? socket_op::create_dgram_or_die(_family) : -1),
            std::bind(&acceptor::event_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            io::EVENT_PERSIST,
            0)
        , impl_(new acceptor_impl { _family, _type, fd(), _port })
#ifdef H_OS_LINUX
        , idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
    {
        assert(idle_fd_ > 0);
#else
    {
#endif
        d_ptr(impl_)->socket_.set_reuse_port(_reuse_port);
        d_ptr(impl_)->socket_.set_reuse_addr(true);
    }

    acceptor::~acceptor()
    {
        if (!tied_object().expired()) {
            tie(nullptr);
            deactivate();
        }
#ifdef H_OS_LINUX
        socket_op::close(idle_fd_);
#endif
        delete impl_;
    }

    auto acceptor::socket() const -> util_socket_t
    {
        return d_ptr(impl_)->socket_.fd();
    }

    auto acceptor::type() const -> TYPE
    {
        return d_ptr(impl_)->socket_.type();
    }

    auto acceptor::port() const -> std::uint16_t
    {
        return d_ptr(impl_)->port_;
    }

    auto acceptor::family() const -> std::uint8_t
    {
        return d_ptr(impl_)->socket_.family();
    }

    void acceptor::event_callback(const io::event::ptr& _event, uint8_t _events, const timestamp& _receive_time)
    {
        assert(this->shared_from_this() == _event);
        if (CHECK_EVENT(_events, io::EVENT_READ) == 0) {
            MSG_ERROR("unexpected operation of acceptor.");
            return;
        }

        host_address peer_addr {};
        auto local_addr = host_address::local_address(socket());

        // FIXME loop until no more ?
        auto accepted { false };
        util_socket_t conn_fd {};

        switch (type()) {
        case TYPE_TCP:
            while ((conn_fd = d_ptr(impl_)->socket_.accept(peer_addr)) >= 0) {
                MSG_TRACE("accepts of tcp[{}].", peer_addr.to_ip_port());
                if (d_ptr(impl_)->new_session_) {
                    d_ptr(impl_)->new_session_(conn_fd, peer_addr, _receive_time, this);
                } else {
                    socket_op::close(conn_fd);
                }
                accepted = true;
            }
            break;
        case TYPE_UDP:
            if (d_ptr(impl_)->new_session_) {
                d_ptr(impl_)->new_session_(-1, local_addr, _receive_time, this);
            }
            accepted = true;
            break;
        default:
            break;
        }

        if (!accepted) {
            MSG_ERROR("cannot accept new connect.");
#ifdef H_OS_LINUX
            // Read the section named "The special problem of
            // accept()ing when you can't" in libev's doc.
            // By Marc Lehmann, author of libev.
            if (errno == EMFILE) {
                socket_op::close(idle_fd_);
                idle_fd_ = socket_op::accept(d_ptr(impl_)->socket_.fd(), nullptr, 0);
                socket_op::close(idle_fd_);
                idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
            }
#endif
        }
    }

    auto acceptor::listen() -> error
    {
        if (owner_cycle() == nullptr) {
            MSG_ERROR("this acceptor[{}] has not been added to any cycle.", (void*)this);
            return error(ERROR_ACCEPTOR_ACTIVED);
        }
        const host_address address(d_ptr(impl_)->port_, false, d_ptr(impl_)->socket_.family() == AF_INET6);
        auto ret = d_ptr(impl_)->socket_.bind_address(address);
        if (!ret) {
            return ret;
        }
        if (type() == TYPE_TCP) {
            ret = d_ptr(impl_)->socket_.listen();
            if (!ret) {
                return ret;
            }
        }
        tie(shared_from_this());
        enable_read();
        return ret;
    }

    void acceptor::set_new_session(new_session _cb)
    {
        d_ptr(impl_)->new_session_ = std::move(_cb);
    }

} // namespace net
} // namespace hare
