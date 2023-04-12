#include <hare/net/hybrid_serve.h>

#include "hare/net/core/cycle.h"
#include <hare/base/logging.h>
#include <hare/net/acceptor.h>
#include <hare/net/tcp_session.h>

namespace hare {
namespace net {

    HybridServe::HybridServe(const std::string& type, const std::string& name)
        : name_(name)
        , reactor_type_(type)
    {
    }

    HybridServe::~HybridServe()
    {
        HARE_ASSERT(!started_, "The hybrid serve is running.");
    }

    auto HybridServe::addAcceptor(const Acceptor::Ptr& acceptor) -> bool
    {
        if (!isRunning()) {
            acceptors_.insert(std::make_pair(acceptor->socket(), acceptor));
            LOG_DEBUG() << "Add acceptor[" << acceptor->socket() << "] to serve[" << this << "].";
        } else {
            cycle_->queueInLoop([=] {
                acceptor->setCycle(cycle_.get());
                acceptor->setNewSessionCallBack(std::bind(&HybridServe::newSession, shared_from_this(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
                auto ret = acceptor->listen();
                if (!ret) {
                    SYS_ERROR() << "acceptor[" << acceptor->socket() << "] cannot listen.";
                    return;
                }
                acceptors_.insert(std::make_pair(acceptor->socket(), acceptor));
                LOG_DEBUG() << "Add acceptor[" << acceptor->socket() << "] to serve[" << this << "].";
            });
        }
        return true;
    }

    void HybridServe::removeAcceptor(util_socket_t acceptor_socket)
    {
        cycle_->queueInLoop([=] {
            auto iter = acceptors_.find(acceptor_socket);
            if (iter == acceptors_.end()) {
                return;
            }
            auto acceptor = std::move(iter->second);
            acceptors_.erase(acceptor_socket);
            LOG_DEBUG() << "Remove acceptor[" << acceptor_socket << "] from serve[" << this << "].";
            acceptor.reset();
        });
    }

    auto HybridServe::add(core::Timer* timer) -> core::Timer::Id
    {
        if (!isRunning()) {
            return 0;
        }

        return cycle_->addTimer(timer);
    }

    void HybridServe::cancel(core::Timer::Id timer_id)
    {
        cycle_->cancel(timer_id);
    }

    void HybridServe::exec()
    {
        cycle_.reset(new core::Cycle(reactor_type_));
        HARE_ASSERT(cycle_, "Cannot create cycle.");

        activeAcceptors();

        started_ = true;
        cycle_->loop();
        started_ = false;

        acceptors_.clear();
        cycle_.reset();
    }

    void HybridServe::exit()
    {
        cycle_->exit();
    }

    void HybridServe::activeAcceptors()
    {
        for (auto iter = acceptors_.begin(); iter != acceptors_.end();) {
            iter->second->setCycle(cycle_.get());
            iter->second->setNewSessionCallBack(std::bind(&HybridServe::newSession, shared_from_this(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            auto ret = iter->second->listen();
            if (!ret) {
                SYS_ERROR() << "socket[" << iter->second->socket() << "] fail to listen.";
                acceptors_.erase(iter++);
                continue;
            }

            LOG_INFO() << "Stream start to listen port[" << iter->second->port() << "].";
            ++iter;
        }
    }

    void HybridServe::newSession(util_socket_t target_fd, HostAddress& address, const Timestamp& time, util_socket_t acceptor_socket)
    {
        HARE_ASSERT(started_ == true, "Serve already stop...");

        auto acceptor_iter = acceptors_.find(acceptor_socket);
        if (acceptor_iter == acceptors_.end()) {
            SYS_ERROR() << "Cannot find specific acceptor";
            return;
        }

        auto acceptor = acceptor_iter->second;
        auto local_addr = HostAddress::localAddress(target_fd);

        char name_buffer[HARE_SMALL_FIXED_SIZE * 2];
        snprintf(name_buffer, sizeof(name_buffer), "-%s#%lu", local_addr.toIpPort().c_str(), session_id_++);
        auto session_name = name_ + name_buffer;

        LOG_DEBUG() << "New session[" << session_name
                    << "] in serve[" << name_
                    << "] from " << address.toIpPort()
                    << " in " << time.toFormattedString();

        Session::Ptr session { nullptr };
        switch (acceptor->type()) {
        case Socket::TYPE_TCP:
            session.reset(new TcpSession(cycle_.get(),
                std::move(local_addr),
                session_name, acceptor->family_, target_fd,
                std::move(address)));
            break;
        case Socket::TYPE_UDP:
        case Socket::TYPE_INVALID:
        default:
            SYS_FATAL() << "Invalid type[" << acceptor->type() << "] of acceptor.";
        }

        newSessionConnected(session, time, acceptor);
        session->connectEstablished();
    }

} // namespace net
} // namespace hare