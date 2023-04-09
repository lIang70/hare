#ifndef _HARE_CORE_STREAM_SESSION_H_
#define _HARE_CORE_STREAM_SESSION_H_

#include <hare/net/tcp_session.h>

#include <functional>
#include <memory>

namespace hare {
namespace core {

    class StreamClient;
    class HARE_API StreamSession : public net::TcpSession {
        using CloseSession = std::function<void(util_socket_t)>;
        using WeakClient = std::weak_ptr<StreamClient>;

        Timestamp login_timestamp_ {};
        CloseSession close_session_ {};
        WeakClient client_ {};

    public:
        using Ptr = std::shared_ptr<StreamSession>;

        ~StreamSession() override;

        inline auto loginTime() -> Timestamp { return login_timestamp_; }
        inline void tiedClient(const std::shared_ptr<StreamClient>& client) { client_ = client; }

    protected:
        explicit StreamSession(net::TcpSessionPrivate* tsp);

        void connection(int32_t flag) override;
        void writeComplete() override;
        void highWaterMark() override;
        void read(net::Buffer& buffer, const Timestamp& time) override;

        friend class StreamServe;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_STREAM_SESSION_H_