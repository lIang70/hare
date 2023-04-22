#include <hare/streaming/kernel/client_rtmp.h>

#include <hare/streaming/protocol/protocol.h>
#include <hare/base/logging.h>

namespace hare {
namespace streaming {

    client_rtmp::client_rtmp(hare::ptr<net::session> _session)
        : client(PROTOCOL_TYPE_RTMP, std::move(_session))
    {
    }

    void client_rtmp::process(io::buffer& buffer, const timestamp& time)
    {
        H_UNUSED(time);
        auto error = protocol()->parse(buffer, session());
        if (!error) {
            SYS_ERROR() << "Failed to parse rtmp packet, detail: " << error.description();
        }
    }

} // namespace streaming
} // namespace hare