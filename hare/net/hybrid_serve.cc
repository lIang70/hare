#include <hare/net/hybrid_serve.h>

#include "hare/net/io_pool.h"
#include "hare/net/socket_op.h"
#include <hare/base/io/cycle.h>
#include <hare/base/logging.h>
#include <hare/net/acceptor.h>
#include <hare/net/tcp_session.h>

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
        if (!is_running()) {
            acceptors_.insert(std::make_pair(_acceptor->socket(), _acceptor));
        } else {
            cycle_->queue_in_cycle([=] {
                cycle_->event_update(_acceptor);
                _acceptor->set_new_session(std::bind(&hybrid_serve::new_session, shared_from_this(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
                auto ret = _acceptor->listen();
                if (!ret) {
                    SYS_ERROR() << "acceptor[" << _acceptor->socket() << "] cannot listen.";
                    return;
                }
                acceptors_.insert(std::make_pair(_acceptor->socket(), _acceptor));
                LOG_DEBUG() << "add acceptor[" << _acceptor->socket() << "] to serve[" << this << "].";
            });
        }
        return true;
    }

    void hybrid_serve::remove_acceptor(util_socket_t _fd)
    {
        cycle_->queue_in_cycle([=] {
            auto iter = acceptors_.find(_fd);
            if (iter == acceptors_.end()) {
                return;
            }
            auto acceptor = std::move(iter->second);
            acceptors_.erase(iter);
            LOG_TRACE() << "remove acceptor[" << _fd << "] from serve[" << this << "].";
            acceptor.reset();
        });
    }

    auto hybrid_serve::exec(int32_t _thread_nbr) -> error
    {
        HARE_ASSERT(cycle_, "cannot create cycle.");

        io_pool_ = std::make_shared<io_pool>("SERVER_WORKER");
        auto ret = io_pool_->start(cycle_->type(), _thread_nbr);
        if (!ret) {
            return error(HARE_ERROR_INIT_IO_POOL);
        }

        active_acceptors();

        started_ = true;
        cycle_->loop();
        started_ = false;

        LOG_TRACE() << "clean io pool...";
        io_pool_->stop();

        acceptors_.clear();
        cycle_.reset();
        io_pool_.reset();

        return error(HARE_ERROR_SUCCESS);
    }

    void hybrid_serve::exit()
    {
        cycle_->exit();
    }

    void hybrid_serve::active_acceptors()
    {
        for (auto iter = acceptors_.begin(); iter != acceptors_.end();) {
            auto& acc = iter->second;
            LOG_TRACE() << "add acceptor[" << acc->socket() << "] to serve[" << this << "].";
            cycle_->event_update(acc);
            acc->set_new_session(std::bind(&hybrid_serve::new_session, shared_from_this(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            auto ret = acc->listen();
            if (!ret) {
                SYS_ERROR() << "socket[" << acc->socket() << "] fail to listen.";
                acceptors_.erase(iter++);
                continue;
            }
            LOG_DEBUG() << "hybrid_serve start to listen port[" << acc->port() << "].";
            ++iter;
        }
    }

    void hybrid_serve::new_session(util_socket_t _fd, host_address& _address, const timestamp& _time, util_socket_t _acceptor)
    {
        HARE_ASSERT(started_ == true, "serve already stop...");
        cycle_->assert_in_cycle_thread();

        auto acceptor_iter = acceptors_.find(_acceptor);
        if (acceptor_iter == acceptors_.end()) {
            SYS_ERROR() << "cannot find specific acceptor";
            return;
        }

        auto acceptor = acceptor_iter->second;
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

        auto next_item = io_pool_->get_next();
        session::ptr session { nullptr };
        switch (acceptor->type()) {
        case TYPE_TCP:
            session.reset(new tcp_session(next_item.cycle.get(),
                std::move(local_addr),
                session_name, acceptor->family_, _fd,
                std::move(_address)));
            break;
        case TYPE_UDP:
        case TYPE_INVALID:
        default:
            SYS_FATAL() << "invalid type[" << acceptor->type() << "] of acceptor.";
        }

        session->destroy_ = std::bind([=] (thread::id _tid) {
            io_pool_->del_session(_tid, _fd);
        }, next_item.thread->tid());

        next_item.cycle->run_in_cycle(std::bind([=] (thread::id _tid) mutable {
            io_pool_->add_session(_tid, _fd, session);
            new_session_(session, _time, acceptor);
            session->connect_established();
        }, next_item.thread->tid()));
    }

} // namespace net
} // namespace hare