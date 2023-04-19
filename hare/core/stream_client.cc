#include <hare/core/stream_client.h>

#include <hare/core/rtmp/protocol_rtmp.h>

namespace hare {
namespace core {

    stream_client::stream_client(PROTOCOL_TYPE _type, hare::ptr<net::session> _session)
        : session_(std::move(_session))
    {
        switch (_type) {
        case PROTOCOL_TYPE_RTMP:
            protocol_ = std::make_shared<protocol_rtmp>();
            break;
        default:
            break;
        }
    }

} // namespace core
} // namespace hare