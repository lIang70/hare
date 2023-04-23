#include <hare/streaming/kernel/client_rtmp.h>

#include <hare/streaming/protocol/protocol.h>
#include <hare/base/logging.h>

namespace hare {
namespace streaming {

    client_rtmp::client_rtmp(util_socket_t _fd)
        : client(PROTOCOL_TYPE_RTMP, _fd)
    {
    }

    void client_rtmp::process(io::buffer& buffer, const hare::ptr<net::session>& _session)
    {
        auto error = protocol()->parse(buffer, _session);
        if (!error) {
            SYS_ERROR() << "failed to parse rtmp packet, detail: " << error.description();
        }
    }

} // namespace streaming
} // namespace hare