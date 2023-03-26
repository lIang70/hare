#ifndef _HARE_CORE_STREAM_SERVE_H_
#define _HARE_CORE_STREAM_SERVE_H_

#include <hare/net/tcp_serve.h>

namespace hare {
namespace core {

    class HARE_API StreamServe : public net::TcpServe {

    public:
        using Ptr = std::shared_ptr<StreamServe>;

        StreamServe(const std::string& type, int8_t family);
        ~StreamServe() override;

        void init(int32_t argc, char** argv);

    protected:
        auto createSession(net::TcpSessionPrivate* tsp) -> net::TcpSession::Ptr override;
        void newConnect(net::TcpSession::Ptr session, Timestamp time) override;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_STREAM_SERVE_H_