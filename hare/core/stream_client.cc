#include <hare/core/stream_client.h>

#include <hare/core/rtmp/protocol_rtmp.h>

namespace hare {
namespace core {

    StreamClient::StreamClient(Type type)
        : client_type_(type)
    {
        switch (client_type_) {
            case TYPE_RTMP:
                protocol_ = std::make_shared<ProtocolRTMP>();
                break;
            default:
                break;
        }
    }

} // namespace core
} // namespace hare