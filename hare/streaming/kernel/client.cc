#include <hare/streaming/kernel/client.h>

#include <hare/streaming/protocol/rtmp/protocol_rtmp.h>

namespace hare {
namespace streaming {

    client::client(PROTOCOL_TYPE _type, hare::ptr<net::session> _session)
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

} // namespace streaming
} // namespace hare