#ifndef _HARE_STREAMING_CLIENT_H_
#define _HARE_STREAMING_CLIENT_H_

#include <hare/base/time/timestamp.h>
#include <hare/streaming/protocol/protocol.h>

namespace hare {
namespace streaming {

    class HARE_API client {
        util_socket_t fd_ { 0 };
        ptr<protocol> protocol_ { nullptr };

    public:
        using ptr = ptr<client>;

        client(PROTOCOL_TYPE _type, util_socket_t _fd);
        virtual ~client() = default;

        inline auto fd() const -> util_socket_t { return fd_; }
        inline auto type() const -> PROTOCOL_TYPE { return protocol_->type(); }
        inline auto protocol() -> hare::ptr<protocol> { return protocol_; }

        virtual void process(io::buffer& buffer, const hare::ptr<net::session>& _session) = 0;
    };

} // namespace streaming
} // namespace hare

#endif // !_HARE_STREAMING_CLIENT_H_