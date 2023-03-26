#include <hare/core/stream_serve.h>
#include <hare/core/stream_session.h>

namespace hare {
namespace core {

    StreamServe::StreamServe(const std::string& type, int8_t family)
        : TcpServe(type, family)
    {

    }

    StreamServe::~StreamServe()
    {
        
    }

    void StreamServe::init(int32_t argc, char** argv)
    {

    }

    net::TcpSession::Ptr StreamServe::createSession(net::TcpSessionPrivate* p)
    {
        return StreamSession::Ptr(new StreamSession(p));
    }

    void StreamServe::newConnect(net::TcpSession::Ptr session, Timestamp ts)
    {

    }

} // namespace core
} // namespace hare 