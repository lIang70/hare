#include <hare/core/rtmp/client.h>

#include <hare/core/protocol.h>
#include <hare/base/logging.h>

namespace hare {
namespace core {

    rtmp_client::rtmp_client(hare::ptr<net::session> _session)
        : stream_client(PROTOCOL_TYPE_RTMP, std::move(_session))
    {
    }

    void rtmp_client::process(io::buffer& buffer, const timestamp& time)
    {
        H_UNUSED(time);
        auto error = protocol()->parse(buffer, session());
        if (!error) {
            SYS_ERROR() << "Failed to parse rtmp packet, detail: " << error.description();
        }
    }

} // namespace core
} // namespace hare