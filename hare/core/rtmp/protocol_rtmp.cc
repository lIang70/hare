#include "hare/base/error.h"
#include <hare/core/rtmp/protocol_rtmp.h>

namespace hare {
namespace core {

    auto ProtocolRTMP::parse(net::Buffer& buffer, StreamSession::Ptr session) -> Error
    {
        return Error(HARE_ERROR_SUCCESS);
    }

} // namespace core
} // namespace hare