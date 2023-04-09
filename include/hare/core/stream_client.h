#ifndef _HARE_CORE_STREAM_CLIENT_H_
#define _HARE_CORE_STREAM_CLIENT_H_

#include <hare/core/protocol.h>
#include <hare/core/stream_session.h>

namespace hare {
namespace core {

    class HARE_API StreamClient {
    public:
        enum Type {
            TYPE_INVALID,
            TYPE_RTMP
        };

    private:
        Type client_type_ { TYPE_INVALID };
        Protocol::Ptr protocol_ { nullptr };
        StreamSession::Ptr session_ { nullptr };

    public:
        using Ptr = std::shared_ptr<StreamClient>;

        explicit StreamClient(Type type);
        virtual ~StreamClient();

        inline auto clientType() -> Type { return client_type_; }
        inline auto protocol() -> Protocol::Ptr { return protocol_; }
        inline void setSession(StreamSession::Ptr session) { session_ = std::move(session); }
        inline auto session() -> StreamSession::Ptr { return session_; }

        virtual void process(net::Buffer& buffer, const Timestamp& time) = 0;

    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_STREAM_CLIENT_H_