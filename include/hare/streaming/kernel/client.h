#ifndef _HARE_STREAMING_CLIENT_H_
#define _HARE_STREAMING_CLIENT_H_

#include <hare/base/time/timestamp.h>
#include <hare/streaming/protocol/protocol.h>

namespace hare {
namespace streaming {

    class HARE_API client {
        ptr<protocol> protocol_ { nullptr };
        ptr<net::session> session_ { nullptr };

    public:
        using ptr = ptr<client>;

        client(PROTOCOL_TYPE _type, hare::ptr<net::session> _session);
        virtual ~client() = default;

        inline auto type() -> PROTOCOL_TYPE { return protocol_->type(); }
        inline auto protocol() -> hare::ptr<protocol> { return protocol_; }
        inline auto session() -> hare::ptr<net::session> { return session_; }

        virtual void process(io::buffer& buffer, const timestamp& time) = 0;
    };

} // namespace streaming
} // namespace hare

#endif // !_HARE_STREAMING_CLIENT_H_