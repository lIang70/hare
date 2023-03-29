#ifndef _HARE_CORE_STREAM_SESSION_H_
#define _HARE_CORE_STREAM_SESSION_H_

#include "hare/base/util/util.h"
#include <hare/base/time/timestamp.h>
#include <hare/net/tcp_session.h>

#include <functional>

namespace hare {
namespace core {

    class HARE_API StreamSession : public net::TcpSession {
        using CloseSession = std::function<void(util_socket_t)>;

        Timestamp login_timestamp_ {};
        CloseSession close_session_ {};

    public:
        using Ptr = std::shared_ptr<StreamSession>;

        ~StreamSession() override;

        inline auto loginTime() -> Timestamp { return login_timestamp_; }

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