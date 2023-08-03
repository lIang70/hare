#include "hare/base/fwd-inl.h"
#include "hare/base/io/socket_op-inl.h"
#include "hare/net/io_pool.h"
#include <hare/base/io/cycle.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/hare-config.h>
#include <hare/net/acceptor.h>
#include <hare/net/hybrid_serve.h>
#include <hare/net/tcp_session.h>

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if defined(H_OS_WIN)
#include <WinSock2.h>
#include <Ws2tcpip.h>
#endif

namespace hare {
namespace net {

    HARE_IMPL_DEFAULT(hybrid_serve,
        std::string name_ {};

        // the acceptor loop
        io::cycle * cycle_ {};
        ptr<io_pool> io_pool_ {};
        std::uint64_t session_id_ { 0 };
        bool started_ { false };

        hybrid_serve::new_session_callback new_session_ {};
    )

    hybrid_serve::hybrid_serve(io::cycle* _cycle, std::string _name)
        : impl_(new hybrid_serve_impl)
    {
        d_ptr(impl_)->name_ = std::move(_name);
        d_ptr(impl_)->cycle_ = _cycle;
    }

    hybrid_serve::~hybrid_serve()
    {
        assert(!d_ptr(impl_)->started_);
        delete impl_;
    }

    auto hybrid_serve::main_cycle() const -> io::cycle*
    {
        return d_ptr(impl_)->cycle_;
    }

    auto hybrid_serve::is_running() const -> bool
    {
        return d_ptr(impl_)->started_;
    }

    void hybrid_serve::set_new_session(new_session_callback _new_session)
    {
        d_ptr(impl_)->new_session_ = std::move(_new_session);
    }

    auto hybrid_serve::add_acceptor(const acceptor::ptr& _acceptor) -> bool
    {
        util::count_down_latch cdl(1);
        auto in_cycle = d_ptr(impl_)->cycle_->in_cycle_thread();
        auto added { false };

        d_ptr(impl_)->cycle_->run_in_cycle([&] {
            d_ptr(impl_)->cycle_->event_update(_acceptor);
            _acceptor->set_new_session(std::bind(&hybrid_serve::new_session, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            auto ret = _acceptor->listen();
            if (!ret) {
                MSG_ERROR("acceptor[{}] cannot listen.", _acceptor->socket());
                cdl.count_down();
                return;
            }

            MSG_TRACE("add acceptor[{}], port={}, type={}] to serve[{}].",
                _acceptor->socket(), _acceptor->port(), socket::type_str(_acceptor->type()), (void*)this);
            added = true;
            cdl.count_down();
        });

        if (!in_cycle) {
            cdl.await();
        }

        return added;
    }

    auto hybrid_serve::exec(std::int32_t _thread_nbr) -> error
    {
        assert(d_ptr(impl_)->cycle_ != nullptr);

        d_ptr(impl_)->io_pool_ = std::make_shared<io_pool>("SERVER_WORKER");
        auto ret = d_ptr(impl_)->io_pool_->start(d_ptr(impl_)->cycle_->type(), _thread_nbr);
        if (!ret) {
            return error(ERROR_INIT_IO_POOL);
        }

        d_ptr(impl_)->started_ = true;
        d_ptr(impl_)->cycle_->loop();
        d_ptr(impl_)->started_ = false;

        MSG_TRACE("clean io pool...");
        d_ptr(impl_)->io_pool_->stop();
        d_ptr(impl_)->io_pool_.reset();

        return error();
    }

    void hybrid_serve::exit()
    {
        d_ptr(impl_)->cycle_->exit();
    }

    void hybrid_serve::new_session(util_socket_t _fd, host_address& _address, const timestamp& _time, acceptor* _acceptor)
    {
        assert(d_ptr(impl_)->started_);
        d_ptr(impl_)->cycle_->assert_in_cycle_thread();

        auto next_item = d_ptr(impl_)->io_pool_->get_next();
        session::ptr session { nullptr };
        std::string name_cache {};

        assert(_fd != -1);
        auto local_addr = host_address::local_address(_fd);

        name_cache = fmt::format("{}-{}#tcp{}", d_ptr(impl_)->name_, local_addr.to_ip_port(), d_ptr(impl_)->session_id_++);

        MSG_TRACE("new session[{}] in serve[{}] from {} in {}.",
            name_cache, d_ptr(impl_)->name_, _address.to_ip_port(), _time.to_fmt());

        session.reset(new tcp_session(next_item->cycle.get(),
            std::move(local_addr),
            name_cache, _acceptor->family(), _fd,
            std::move(_address)));

        if (!session) {
            MSG_ERROR("fail to create session[{}].", name_cache);
            socket_op::close(_fd);
            return;
        }

        auto sfd = session->fd();

        session->set_destroy(std::bind([=]() {
            next_item->cycle->run_in_cycle([=]() mutable {
                assert(next_item->sessions.find(sfd) != next_item->sessions.end());
                next_item->sessions.erase(sfd);
            });
        }));

        next_item->cycle->run_in_cycle(std::bind([=]() mutable {
            assert(next_item->sessions.find(sfd) == next_item->sessions.end());
            if (!d_ptr(impl_)->new_session_) {
                MSG_ERROR("you need register new_session_callback to serve[{}].", d_ptr(impl_)->name_);
                if (sfd != -1) {
                    socket_op::close(sfd);
                }
                return;
            }
            next_item->sessions.insert(std::make_pair(sfd, session));
            d_ptr(impl_)->new_session_(session, _time, std::static_pointer_cast<acceptor>(_acceptor->shared_from_this()));
            session->connect_established();
        }));
    }

} // namespace net
} // namespace hare