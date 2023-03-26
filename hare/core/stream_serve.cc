#include <hare/base/logging.h>
#include <hare/core/stream_serve.h>
#include <hare/core/stream_session.h>
#include <hare/net/acceptor.h>
#include <memory>
#include <sys/socket.h>

namespace hare {
namespace core {

    namespace detail {
        void help()
        {
        }
    }

    StreamServe::StreamServe(const std::string& type)
        : TcpServe(type, "STREAM_SERVE")
    {
    }

    StreamServe::~StreamServe()
    {
        LOG_DEBUG() << "";
    }

    void StreamServe::init(int32_t argc, char** argv)
    {
        for (auto i = 0; i < argc; ++i) {
            if (std::string(argv[i]) == "--listen" && i + 1 < argc) {
                auto listen_port = std::stoi(std::string(argv[i + 1]));
                net::Acceptor::Ptr acceptor = std::make_shared<net::Acceptor>(AF_INET, listen_port);
                add(acceptor);
            }
        }
    }

    auto StreamServe::createSession(net::TcpSessionPrivate* tsp) -> net::TcpSession::Ptr
    {
        return StreamSession::Ptr(new StreamSession(tsp));
    }

    void StreamServe::newConnect(net::TcpSession::Ptr session, Timestamp time)
    {
    }

} // namespace core
} // namespace hare