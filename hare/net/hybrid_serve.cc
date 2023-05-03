#include <hare/net/hybrid_serve.h>

#include "hare/net/io_pool.h"
#include "hare/net/socket_op.h"
#include <hare/base/io/cycle.h>
#include <hare/base/logging.h>
#include <hare/net/acceptor.h>
#include <hare/net/tcp_session.h>
#include <hare/net/udp_session.h>

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

namespace hare {
namespace net {

    hybrid_serve::hybrid_serve(hare::ptr<io::cycle> _cycle, std::string _name)
        : name_(std::move(_name))
        , cycle_(std::move(_cycle))
    {
    }

    hybrid_serve::~hybrid_serve()
    {
        HARE_ASSERT(!started_, "the hybrid serve is running.");
    }

    auto hybrid_serve::add_acceptor(const acceptor::ptr& _acceptor) -> bool
    {
        cycle_->run_in_cycle([=] {
            cycle_->event_update(_acceptor);
            _acceptor->set_new_session(std::bind(&hybrid_serve::new_session, shared_from_this(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            auto ret = _acceptor->listen();
            if (!ret) {
                SYS_ERROR() << "acceptor[" << _acceptor->socket() << "] cannot listen.";
                return;
            }
            LOG_DEBUG() << "add acceptor[" << _acceptor->socket()
                        << ", port=" << _acceptor->port_
                        << ", type=" << socket::type_str(_acceptor->type())
                        << "] to serve[" << this << "].";
        });
        return true;
    }

    auto hybrid_serve::exec(int32_t _thread_nbr) -> error
    {
        HARE_ASSERT(cycle_, "cannot create cycle.");

        io_pool_ = std::make_shared<io_pool>("SERVER_WORKER");
        auto ret = io_pool_->start(cycle_->type(), _thread_nbr);
        if (!ret) {
            return error(HARE_ERROR_INIT_IO_POOL);
        }

        started_ = true;
        cycle_->loop();
        started_ = false;

        LOG_TRACE() << "clean io pool...";
        io_pool_->stop();

        cycle_.reset();
        io_pool_.reset();

        return error(HARE_ERROR_SUCCESS);
    }

    void hybrid_serve::exit()
    {
        cycle_->exit();
    }

    void hybrid_serve::new_session(util_socket_t _fd, host_address& _address, const timestamp& _time, acceptor* _acceptor)
    {
        HARE_ASSERT(started_ == true, "serve already stop...");
        cycle_->assert_in_cycle_thread();

        auto next_item = io_pool_->get_next();
        session::ptr session { nullptr };

        switch (_acceptor->type()) {
        case TYPE_TCP: {
            HARE_ASSERT(_fd != -1, "socket error.");
            auto local_addr = host_address::local_address(_fd);

            std::array<char, static_cast<size_t>(HARE_SMALL_FIXED_SIZE * 2)> cache {};
            snprintf(cache.data(), cache.size(), "-%s#%lu", local_addr.to_ip_port().c_str(), session_id_++);
            auto session_name = name_ + cache.data();

            LOG_DEBUG() << "new session[" << session_name
                        << "] in serve[" << name_
                        << "] from " << _address.to_ip_port()
                        << " in " << _time.to_fmt();

            if (!new_session_) {
                SYS_ERROR() << "you need register new_session_callback to serve[" << name_ << "]";
                socket_op::close(_fd);
                return;
            }

            session.reset(new tcp_session(next_item->cycle.get(),
                std::move(local_addr),
                session_name, _acceptor->family_, _fd,
                std::move(_address)));
        } break;
        case TYPE_UDP: {
            HARE_ASSERT(_fd == -1, "socket error.");

            struct sockaddr_in6 addr { };
            size_t addr_len = socket_op::get_addr_len(_acceptor->family_);
            /// FIXME: recv the first package. 
            socket_op::recvfrom(_acceptor->socket(), nullptr, 0, sockaddr_cast(&addr), addr_len);

            host_address peer_addr {};
            peer_addr.set_sockaddr_in6(&addr);

            auto accept_fd = socket_op::create_dgram_or_die(_acceptor->family_);
            auto reuse { 1 };
            auto ret = ::setsockopt(accept_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
            if (ret < 0) {
                socket_op::close(accept_fd);
            }
            ret = ::setsockopt(accept_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
            if (ret < 0) {
                socket_op::close(accept_fd);
            }
            auto err = socket_op::bind(accept_fd, _address.get_sockaddr());
            if (!err) {
                socket_op::close(accept_fd);
            }
            err = socket_op::connect(accept_fd, sockaddr_cast(&addr));
            if (!err) {
                socket_op::close(accept_fd);
            }

            std::array<char, static_cast<size_t>(HARE_SMALL_FIXED_SIZE * 2)> cache {};
            snprintf(cache.data(), cache.size(), "-%s#%lu", _address.to_ip_port().c_str(), session_id_++);
            auto session_name = name_ + cache.data();

            LOG_DEBUG() << "new session[" << session_name
                        << "] in serve[" << name_
                        << "] from " << peer_addr.to_ip_port()
                        << " in " << _time.to_fmt();
            
            session.reset(new udp_session(next_item->cycle.get(),
                std::move(_address),
                session_name, _acceptor->family_, _fd,
                std::move(peer_addr)));
        } break;
        default:
            SYS_FATAL() << "invalid type[" << _acceptor->type() << "] of acceptor.";
        }

        session->destroy_ = std::bind([=]() {
            next_item->cycle->run_in_cycle([&]() mutable {
                HARE_ASSERT(next_item->sessions.find(_fd) != next_item->sessions.end(), "");
                next_item->sessions.erase(_fd);
            });
        });

        next_item->cycle->run_in_cycle(std::bind([=]() mutable {
            HARE_ASSERT(next_item->sessions.find(_fd) == next_item->sessions.end(), "");
            next_item->sessions.insert(std::make_pair(_fd, session));
            new_session_(session, _time, std::static_pointer_cast<acceptor>(_acceptor->shared_from_this()));
            session->connect_established();
        }));
    }

} // namespace net
} // namespace hare