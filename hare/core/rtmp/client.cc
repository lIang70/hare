#include "hare/base/util.h"
#include <hare/core/rtmp/client.h>

#include <hare/core/stream_session.h>
#include <hare/core/protocol.h>
#include <hare/base/logging.h>

namespace hare {
namespace core {

    RTMPClient::RTMPClient()
        : StreamClient(TYPE_RTMP)
    {
    }

    void RTMPClient::process(net::Buffer& buffer, const Timestamp& time)
    {
        H_UNUSED(time);
        auto error = protocol()->parse(buffer, session());
        if (!error) {
            LOG_ERROR() << "Failed to parse rtmp packet, detail: " << error.description();
        }
    }

} // namespace core
} // namespace hare