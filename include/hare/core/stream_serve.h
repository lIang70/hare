#ifndef _HARE_CORE_STREAM_SERVE_H_
#define _HARE_CORE_STREAM_SERVE_H_

#include <hare/net/tcp_serve.h>

namespace hare {
namespace core {

    class StreamServePrivate;
    class HARE_API StreamServe : public net::TcpServe {
        StreamServePrivate* p_ { nullptr };

    public:
        using Ptr = std::shared_ptr<StreamServe>;

        explicit StreamServe(const std::string& type);
        ~StreamServe() override;

    protected:
        auto createSession(net::TcpSessionPrivate* tsp) -> net::TcpSession::Ptr override;
        void newSession(net::TcpSession::Ptr session, Timestamp time, const net::Acceptor::Ptr& acceptor) override;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_STREAM_SERVE_H_