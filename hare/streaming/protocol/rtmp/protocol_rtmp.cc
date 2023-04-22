#include <hare/core/rtmp/protocol_rtmp.h>

namespace hare {
namespace core {

    // inline auto isAudio() const -> bool { return message_type_ == AUDIO_MESSAGE; }
    // inline auto isVideo() const -> bool { return message_type_ == VIDEO_MESSAGE; }
    // inline auto isAMF0Command() const -> bool { return message_type_ == AMF0_COMMAND_MESSAGE; }
    // inline auto isAMF0Data() const -> bool { return message_type_ == AMF0_DATA_MESSAGE; }
    // inline auto isAMF3Command() const -> bool { return message_type_ == AMF3_COMMAND_MESSAGE; }
    // inline auto isAMF3Data() const -> bool { return message_type_ == AMF3_DATA_MESSAGE; }
    // inline auto isWindowAckledgementSize() const -> bool { return message_type_ == WINDOW_ACKNOWLEDGEMENT_SIZE; }
    // inline auto isSetChunkSize() const -> bool { return message_type_ == SET_CHUNK_SIZE; }
    // inline auto isUserControlMessage() const -> bool { return message_type_ == USER_CONTROL_MESSAGE; }

    protocol_rtmp::protocol_rtmp()
        : protocol(PROTOCOL_TYPE_RTMP)
        , hand_shark_(new complex_handshake)
    {
    }

    auto protocol_rtmp::parse(io::buffer& buffer, hare::ptr<net::session> session) -> error
    {
        auto err = hand_shark_->hand_shake_client(buffer, session);
        return err;
    }

} // namespace core
} // namespace hare