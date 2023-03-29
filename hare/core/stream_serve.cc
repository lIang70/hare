#include "hare/base/util/util.h"
#include "hare/net/tcp_session.h"
#include <functional>
#include <hare/base/logging.h>
#include <hare/core/stream_serve.h>
#include <hare/core/stream_session.h>

#include <map>
#include <mutex>

namespace hare {
namespace core {

    class StreamServePrivate {

        std::mutex sessions_mutex_ {};
        std::map<util_socket_t, net::TcpSession::Ptr> sessions_ {};

    public:
        auto addSession(net::TcpSession::Ptr session) -> bool
        {
            std::unique_lock<std::mutex> lock(sessions_mutex_);
            auto iter = sessions_.find(session->socket());
            if (iter == sessions_.end()) {
                sessions_.emplace(session->socket(), session);
                return true;
            }
            return false;
        }

        void delSession(util_socket_t socket)
        {
            net::TcpSession::Ptr session { nullptr };
            {
                std::unique_lock<std::mutex> lock(sessions_mutex_);
                auto iter = sessions_.find(socket);
                if (iter != sessions_.end()) {
                    session = std::move(iter->second);
                    sessions_.erase(iter);
                } else {
                    lock.unlock();
                    LOG_INFO() << "Cannot find session[" << socket << "]";
                }
            }
        }
    };

    StreamServe::StreamServe(const std::string& type)
        : TcpServe(type, "HARE")
        , p_(new StreamServePrivate)
    {
    }

    StreamServe::~StreamServe()
    {
        LOG_DEBUG() << "Stream serve is deleted.";
        delete p_;
    }

    auto StreamServe::createSession(net::TcpSessionPrivate* tsp) -> net::TcpSession::Ptr
    {
        return StreamSession::Ptr(new StreamSession(tsp));
    }

    void StreamServe::newSession(net::TcpSession::Ptr session, Timestamp time)
    {
        LOG_INFO() << "New session[" << session->name()
                   << "] in " << time.toFormattedString();
        auto* stream_session = dynamic_cast<StreamSession*>(session.get());
        if (stream_session == nullptr) {
            LOG_ERROR() << "Cannot get StreamSession";
            return;
        }

        stream_session->login_timestamp_.swap(time);
        stream_session->close_session_ = std::bind(&StreamServePrivate::delSession, p_, std::placeholders::_1);

        auto ret = p_->addSession(session);
        if (!ret) {
            LOG_ERROR() << "The same socket[" << session->socket() << "] already exists...";
        }
    }

} // namespace core
} // namespace hare