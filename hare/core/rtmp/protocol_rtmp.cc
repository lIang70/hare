#include "hare/base/error.h"
#include "hare/core/protocol.h"
#include "hare/core/rtmp/hand_shake.h"
#include <hare/core/rtmp/protocol_rtmp.h>

namespace hare {
namespace core {

    ProtocolRTMP::ProtocolRTMP()
        : hand_shark_(new CpxHandShake)
    {
    }

    auto ProtocolRTMP::parse(net::Buffer& buffer, StreamSession::Ptr session) -> Error
    {
        auto err = hand_shark_->handShakeWithClient(buffer, session);
        return err;
    }

} // namespace core
} // namespace hare