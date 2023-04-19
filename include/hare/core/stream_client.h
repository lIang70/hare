#ifndef _HARE_CORE_STREAM_CLIENT_H_
#define _HARE_CORE_STREAM_CLIENT_H_

#include <hare/base/time/timestamp.h>
#include <hare/core/protocol.h>

namespace hare {
namespace core {

    class HARE_API stream_client {
        ptr<protocol> protocol_ { nullptr };
        ptr<net::session> session_ { nullptr };

    public:
        using ptr = ptr<stream_client>;

        stream_client(PROTOCOL_TYPE _type, hare::ptr<net::session> _session);
        virtual ~stream_client() = default;

        inline auto type() -> PROTOCOL_TYPE { return protocol_->type(); }
        inline auto protocol() -> hare::ptr<protocol> { return protocol_; }
        inline auto session() -> hare::ptr<net::session> { return session_; }

        virtual void process(io::buffer& buffer, const timestamp& time) = 0;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_STREAM_CLIENT_H_