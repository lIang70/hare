#include <hare/net/acceptor.h>

#include "hare/base/io/reactor.h"
#include "hare/net/socket_op.h"
#include <hare/net/socket.h>
#include <hare/base/logging.h>

#if HARE__HAVE_FCNTL_H
#include <fcntl.h>
#endif // HARE__HAVE_FCNTL_H

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif // HARE__HAVE_NETINET_IN_H

namespace hare {
namespace net {

    acceptor::acceptor(int8_t _family, TYPE _type, int16_t _port, bool _reuse_port)
        : io::event(_type == TYPE_TCP ? 
                         socket_op::create_nonblocking_or_die(_family) : (_type == TYPE_UDP ? 
                                                                   socket_op::create_dgram_or_die(_family) : -1),
                std::bind(&acceptor::event_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                io::EVENT_PERSIST,
                0)
        , socket_(_family, _type, fd())
        , family_(_family)
        , port_(_port)
#ifdef H_OS_LINUX
        , idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
    {
        HARE_ASSERT(idle_fd_ > 0, "cannot open idel fd");
#else
    {
#endif
        socket_.set_reuse_port(_reuse_port);
        socket_.set_reuse_addr(true);
    }

    acceptor::~acceptor()
    {
        tie(nullptr);
        deactivate();
#ifdef H_OS_LINUX
        socket_op::close(idle_fd_);
#endif
    }

    void acceptor::event_callback(const io::event::ptr& _event, uint8_t _events, const timestamp& _receive_time)
    {
        HARE_ASSERT(this->shared_from_this() == _event, "error event callback");
        if (CHECK_EVENT(_events, io::EVENT_READ) == 0){
            SYS_ERROR() << "unexpected operation of acceptor.";
            return;
        }
        
        host_address peer_addr {};
        auto local_addr = host_address::local_address(socket());
        
        // FIXME loop until no more ?
        auto accepted { false };
        util_socket_t conn_fd {};
        
        switch (type()) {
            case TYPE_TCP: 
            if ((conn_fd = socket_.accept(peer_addr)) >= 0) {
                LOG_TRACE() << "accepts of " << peer_addr.to_ip_port();
                if (new_session_) {
                    new_session_(conn_fd, peer_addr, _receive_time, this);
                } else {
                    socket_op::close(conn_fd);
                }
                accepted = true;
            }
                break;
            case TYPE_UDP:
            if ((conn_fd = socket_.accept(peer_addr, &local_addr)) >= 0) {
                LOG_TRACE() << "accepts of " << peer_addr.to_ip_port();
                if (new_session_) {
                    new_session_(conn_fd, peer_addr, _receive_time, this);
                } else {
                    socket_op::close(conn_fd);
                }
                accepted = true;
            }
                break;
            default:
                break;
        }
        
        if (!accepted) {
            SYS_ERROR() << "cannot accept new connect";
#ifdef H_OS_LINUX
            // Read the section named "The special problem of
            // accept()ing when you can't" in libev's doc.
            // By Marc Lehmann, author of libev.
            if (errno == EMFILE) {
                socket_op::close(idle_fd_);
                idle_fd_ = socket_op::accept(socket_.fd(), nullptr);
                socket_op::close(idle_fd_);
                idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
            }
#endif
        }
    }

    auto acceptor::listen() -> error
    {
        if (owner_cycle() == nullptr) {
            SYS_ERROR() << "this acceptor[" << this << "] has not been added to any cycle.";
            return error(HARE_ERROR_ACCEPTOR_ACTIVED);
        }
        const host_address address(port_, false, family_ == AF_INET6);
        auto ret = socket_.bind_address(address);
        if (!ret) {
            return ret;
        }
        if (type() == TYPE_TCP) {
            ret = socket_.listen();
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
        new_session_ = std::move(_cb);
    }

} // namespace net
} // namespace hare