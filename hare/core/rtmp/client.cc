#include <hare/core/rtmp/client.h>

#include <hare/core/stream_session.h>
#include <hare/core/protocol.h>

namespace hare {
namespace core {

    RTMPClient::RTMPClient()
        : StreamClient(TYPE_RTMP)
    {
    }

    void RTMPClient::process(net::Buffer& buffer, const Timestamp& time)
    {
        if (protocol()) {

        }
    }

} // namespace core
} // namespace hare