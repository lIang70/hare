#include "hare/base/fwd-inl.h"
#include "hare/net/io_pool.h"
#include "hare/net/socket_op.h"
#include <hare/base/io/cycle.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/net/acceptor.h>
#include <hare/net/hybrid_serve.h>
#include <hare/net/tcp_session.h>
#include <hare/net/udp_session.h>

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

namespace hare {
namespace net {

    namespace detail {

        auto create_udp_socket(host_address& local_addr, int8_t _family, host_address& peer_addr) -> util_socket_t
        {
            auto accept_fd = socket_op::create_dgram_or_die(_family);
            if (accept_fd <= 0) {
                return accept_fd;
            }

            auto reuse { 1 };
            auto ret = ::setsockopt(accept_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
            if (ret < 0) {
                MSG_ERROR("cannot set address-reuse to socket.");
                socket_op::close(accept_fd);
                return -1;
            }

            ret = ::setsockopt(accept_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
            if (ret < 0) {
                MSG_ERROR("cannot set address-port to socket.");
                socket_op::close(accept_fd);
                return -1;
            }

            auto err = socket_op::bind(accept_fd, local_addr.get_sockaddr());
            if (!err) {
                MSG_ERROR("socket[{}] cannot bind address {}.", accept_fd, local_addr.to_ip_port());
                socket_op::close(accept_fd);
                return -1;
            }

            err = socket_op::connect(accept_fd, peer_addr.get_sockaddr());
            if (!err) {
                MSG_ERROR("socket[{}] cannot connect address {}.", accept_fd, local_addr.to_ip_port());
                socket_op::close(accept_fd);
                return -1;
            }

            return accept_fd;
        }

    } // namespace detail

    hybrid_serve::hybrid_serve(io::cycle* _cycle, std::string _name)
        : name_(std::move(_name))
        , cycle_(_cycle)
    {
    }

    hybrid_serve::~hybrid_serve()
    {
        assert(!started_);
    }

    auto hybrid_serve::add_acceptor(const acceptor::ptr& _acceptor) -> bool
    {
        util::count_down_latch cdl(1);
        auto in_cycle = cycle_->in_cycle_thread();
        auto added { false };

        cycle_->run_in_cycle([&] {
            cycle_->event_update(_acceptor);
            _acceptor->set_new_session(std::bind(&hybrid_serve::new_session, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            auto ret = _acceptor->listen();
            if (!ret) {
                MSG_ERROR("acceptor[{}] cannot listen.", _acceptor->socket());
                cdl.count_down();
                return;
            }

            MSG_TRACE("add acceptor[{}], port={}, type={}] to serve[{}].",
                _acceptor->socket(), _acceptor->port_, socket::type_str(_acceptor->type()), (void*)this);
            added = true;
            cdl.count_down();
        });

        if (!in_cycle) {
            cdl.await();
        }

        return added;
    }

    auto hybrid_serve::exec(int32_t _thread_nbr) -> error
    {
        assert(cycle_);

        io_pool_ = std::make_shared<io_pool>("SERVER_WORKER");
        auto ret = io_pool_->start(cycle_->type(), _thread_nbr);
        if (!ret) {
            return error(ERROR_INIT_IO_POOL);
        }

        started_ = true;
        cycle_->loop();
        started_ = false;

        MSG_TRACE("clean io pool...");
        io_pool_->stop();
        io_pool_.reset();

        return error();
    }

    void hybrid_serve::exit()
    {
        cycle_->exit();
    }

    void hybrid_serve::new_session(util_socket_t _fd, host_address& _address, const timestamp& _time, acceptor* _acceptor)
    {
        assert(started_ == true);
        cycle_->assert_in_cycle_thread();

        if (!new_session_) {
            MSG_TRACE("you need register new_session_callback to serve[{}].", name_);
            if (_fd != -1) {
                socket_op::close(_fd);
            }
            return;
        }

        auto next_item = io_pool_->get_next();
        session::ptr session { nullptr };
        auto type = _acceptor->type();
        std::string name_cache {};

        switch (type) {
        case TYPE_TCP: {
            assert(_fd != -1);
            auto local_addr = host_address::local_address(_fd);

            name_cache = fmt::format("{}-{}#tcp{}", name_, local_addr.to_ip_port(), session_id_++);

            MSG_TRACE("new session[{}] in serve[{}] from {} in {}.",
                name_cache, name_, _address.to_ip_port(), _time.to_fmt());

            session.reset(new tcp_session(next_item->cycle.get(),
                std::move(local_addr),
                name_cache, _acceptor->family_, _fd,
                std::move(_address)));
        } break;
        case TYPE_UDP: {
            assert(_fd == -1);

            struct sockaddr_in6 addr { };
            size_t addr_len = socket_op::get_addr_len(_acceptor->family_);
            auto buffer_size = socket_op::get_bytes_readable_on_socket(_acceptor->socket());
            auto* buffer = new uint8_t[buffer_size];
            auto recv_size = socket_op::recvfrom(_acceptor->socket(), buffer, buffer_size, sockaddr_cast(&addr), addr_len);
            assert(recv_size == buffer_size);

            host_address peer_addr {};
            peer_addr.set_sockaddr_in6(&addr);

            auto accept_fd = detail::create_udp_socket(_address, _acceptor->family_, peer_addr);

            name_cache = fmt::format("{}-{}#udp{}", name_, peer_addr.to_ip_port(), session_id_++);

            MSG_TRACE("new session[{}] in serve[{}] from {} in {}.",
                name_cache, name_, peer_addr.to_ip_port(), _time.to_fmt());

            session.reset(new udp_session(next_item->cycle.get(),
                std::move(_address),
                name_cache, _acceptor->family_, accept_fd,
                std::move(peer_addr)));

            auto udp = std::static_pointer_cast<udp_session>(session);
            udp->in_buffer_.add_block(buffer, buffer_size);

        } break;
        default:
            MSG_FATAL("invalid type[{}] of acceptor.", type_to_str(_acceptor->type()));
        }

        if (!session) {
            return;
        }

        session->destroy_ = std::bind([=]() {
            next_item->cycle->run_in_cycle([=]() mutable {
                assert(next_item->sessions.find(_fd) != next_item->sessions.end());
                next_item->sessions.erase(_fd);
            });
        });

        next_item->cycle->run_in_cycle(std::bind([=]() mutable {
            assert(next_item->sessions.find(_fd) == next_item->sessions.end());
            next_item->sessions.insert(std::make_pair(_fd, session));
            new_session_(session, _time, std::static_pointer_cast<acceptor>(_acceptor->shared_from_this()));
            session->connect_established();

            if (type == TYPE_UDP) {
                auto udp = std::static_pointer_cast<udp_session>(session);
                if (udp->read_) {
                    udp->read_(udp, udp->in_buffer_, _time);
                }
            }
        }));
    }

} // namespace net
} // namespace hare