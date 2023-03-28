#include "hare/net/core/cycle.h"
#include "hare/net/tcp_serve_p.h"
#include "hare/net/tcp_session_p.h"
#include <hare/base/logging.h>
#include <hare/base/util/util.h>

namespace hare {
namespace net {

    void TcpServePrivate::newSession(util_socket_t target_fd, int8_t family, const HostAddress& address, const Timestamp& time)
    {
        HARE_ASSERT(started_ == true, "Serve already stop...");
        cycle_->assertInCycleThread();

        auto* work_cycle = thread_pool_->getNextCycle();
        char buf[HARE_SMALL_FIXED_SIZE * 2];
        auto local_addr = HostAddress::localAddress(target_fd);

        snprintf(buf, sizeof(buf), "-%s#%lu", local_addr.toIpPort().c_str(), session_id_++);
        auto session_name = name_ + buf;
        LOG_DEBUG() << "New session[" << session_name
                    << "] in serve[" << name_
                    << "] from " << address.toIpPort()
                    << " in " << time.toFormattedString();

        auto* tsp = new TcpSessionPrivate(work_cycle, session_name, family, target_fd, local_addr, address);

        if (serve_ == nullptr) {
            delete tsp;
            SYS_ERROR() << "Cannot get TcpServe...";
        } else {
            auto session = serve_->createSession(tsp);
            work_cycle->runInLoop(std::bind(&TcpSession::connectEstablished, session));
            work_cycle->runInLoop(std::bind(&TcpServe::newConnect, serve_->shared_from_this(), session, time));
        }
    }

    TcpServe::TcpServe(const std::string& type, const std::string& name)
        : p_(new TcpServePrivate)
    {
        p_->serve_ = this;
        p_->reactor_type_ = type;
        p_->name_ = name;
    }

    TcpServe::~TcpServe()
    {
        HARE_ASSERT(p_ != nullptr && !p_->started_, "");
        delete p_;
    }

    void TcpServe::setThreadNum(int32_t num)
    {
        HARE_ASSERT(p_->started_ == false, "The serve already running...");
        p_->thread_num_ = num;
    }

    auto TcpServe::isRunning() const -> bool
    {
        return p_->started_.load();
    }

    auto TcpServe::add(const Acceptor::Ptr& acceptor) -> bool
    {
        if (!isRunning()) {
            p_->acceptors_.insert(std::make_pair(acceptor->socket(), acceptor));
        } else {
            p_->acceptors_.insert(std::make_pair(acceptor->socket(), acceptor));
            acceptor->setCycle(p_->cycle_.get());
            acceptor->setNewSessionCallBack(std::bind(&TcpServePrivate::newSession, p_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            auto ret = acceptor->listen();
            if (!ret) {
                p_->acceptors_.erase(acceptor->socket());
                return false;
            }
        }
        LOG_DEBUG() << "Add acceptor[" << acceptor->socket() << "] to serve[" << this << "].";
        return true;
    }

    auto TcpServe::add(net::Timer* timer) -> TimerId
    {
        if (!isRunning()) {
            return 0;
        }

        // FIXME sync in other thread
        return p_->cycle_->addTimer(timer);
    }

    void TcpServe::cancel(net::TimerId timer_id)
    {
        if (p_->cycle_->isInCycleThread()) {
            p_->cycle_->cancel(timer_id);
        } else {
            p_->cycle_->runInLoop(std::bind(&core::Cycle::cancel, p_->cycle_.get(), timer_id));
        }
    }

    void TcpServe::exec()
    {
        p_->cycle_.reset(new core::Cycle(p_->reactor_type_));
        HARE_ASSERT(p_->cycle_ != nullptr, "Cannot create cycle.");

        activeAcceptors();

        p_->thread_pool_ = std::make_shared<core::CycleThreadPool>(p_->cycle_.get(), p_->reactor_type_, p_->name_ + "_WORKER");
        p_->thread_pool_->setThreadNum(p_->thread_num_);
        p_->thread_pool_->start();

        p_->started_.exchange(true);
        p_->cycle_->loop();
        p_->started_.exchange(false);

        p_->acceptors_.clear();
        p_->thread_pool_.reset();
        p_->cycle_.reset();
    }

    void TcpServe::exit()
    {
        p_->cycle_->exit();
    }

    void TcpServe::activeAcceptors()
    {
        for (auto iter = p_->acceptors_.begin(); iter != p_->acceptors_.end();) {
            iter->second->setCycle(p_->cycle_.get());
            iter->second->setNewSessionCallBack(std::bind(&TcpServePrivate::newSession, p_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            auto ret = iter->second->listen();
            if (!ret) {
                SYS_ERROR() << "socket[" << iter->second->socket() << "] fail to listen.";
                p_->acceptors_.erase(iter++);
                continue;
            }

            LOG_INFO() << "Stream start to listen port[" << iter->second->port() << "].";
            ++iter;
        }
    }

} // namespace net
} // namespace hare