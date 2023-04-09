#include <hare/base/logging.h>
#include <hare/core/stream_serve.h>
#include <hare/core/stream_session.h>
#include <hare/core/rtmp/client.h>
#include <hare/net/tcp_session.h>

#include <map>
#include <mutex>

namespace hare {
namespace core {

    namespace detail {

        struct SessionNotFree {
            void operator() (StreamSession* session)
            {
                H_UNUSED(session);
            }
        };

    } // namespace detail

    class StreamServePrivate {

        std::mutex sessions_mutex_ {};
        std::map<util_socket_t, StreamClient::Ptr> sessions_ {};

    public:
        auto addClient(StreamClient::Ptr client) -> bool
        {
            std::unique_lock<std::mutex> lock(sessions_mutex_);
            auto iter = sessions_.find(client->session()->socket());
            if (iter == sessions_.end()) {
                sessions_.emplace(client->session()->socket(), client);
                return true;
            }
            return false;
        }

        void removeClient(util_socket_t socket)
        {
            StreamClient::Ptr client { nullptr };
            {
                std::unique_lock<std::mutex> lock(sessions_mutex_);
                auto iter = sessions_.find(socket);
                if (iter != sessions_.end()) {
                    client = std::move(iter->second);
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
        return {new StreamSession(tsp), detail::SessionNotFree()};
    }

    void StreamServe::newSession(net::TcpSession::Ptr session, Timestamp time, const net::Acceptor::Ptr& acceptor)
    {
        LOG_INFO() << "New session[" << session->name()
                   << "] in " << time.toFormattedString();
        auto* ssession = dynamic_cast<StreamSession*>(session.get());
        if (ssession == nullptr) {
            LOG_ERROR() << "Cannot get StreamSession";
            delete session.get();
            return;
        }

        StreamSession::Ptr stream_session(ssession);
        stream_session->login_timestamp_.swap(time);
        stream_session->close_session_ = std::bind(&StreamServePrivate::removeClient, p_, std::placeholders::_1);

        StreamClient::Ptr client = std::make_shared<RTMPClient>();
        client->setSession(stream_session);

        stream_session->tiedClient(client);

        auto ret = p_->addClient(client);
        if (!ret) {
            LOG_ERROR() << "The same socket[" << session->socket() << "] already exists...";
        }
    }

} // namespace core
} // namespace hare