#ifndef _HARE_CORE_STREAM_SESSION_H_
#define _HARE_CORE_STREAM_SESSION_H_

#include <hare/net/tcp_session.h>

namespace hare {
namespace core {

    class HARE_API StreamSession : public net::TcpSession {
        friend class StreamServe;

    public:
        using Ptr = std::shared_ptr<StreamSession>;

        ~StreamSession() override;

    protected:
        explicit StreamSession(net::TcpSessionPrivate* tsp);
        
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_STREAM_SESSION_H_