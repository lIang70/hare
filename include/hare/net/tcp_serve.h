#ifndef _HARE_NET_TCP_SERVE_H_
#define _HARE_NET_TCP_SERVE_H_

#include <hare/net/tcp_session.h>

namespace hare {
namespace net {

    class TcpServePrivate;
    class TcpServe : public NonCopyable
                   , public std::enable_shared_from_this<TcpServe> {
        TcpServePrivate* p_ { nullptr };

    public:
        TcpServe();
        virtual ~TcpServe();

    protected:
        virtual inline STcpSession createSession(TcpSessionPrivate* p)
        {
            return STcpSession(new TcpSession(p));
        }

        virtual void newConnect(STcpSession& session) = 0;
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SERVE_H_