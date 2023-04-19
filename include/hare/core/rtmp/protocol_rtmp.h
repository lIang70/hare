#ifndef _HARE_CORE_PROTOCOL_RTMP_H_
#define _HARE_CORE_PROTOCOL_RTMP_H_

#include <hare/core/protocol.h>
#include <hare/core/rtmp/hand_shake.h>

/**
 * @code 
 * +-+-+-+-+-+-+-+-+-+-+- ... -+-+-+
 * |basic header | message header  |
 * +-+-+-+-+-+-+-+-+-+-+- ... -+-+-+
 * | extended timestamp            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | extended timestamp (cont)     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endcode
 */

// Chunks of Type 0 are 11 bytes long. This type MUST be used at the
//  start of a chunk stream, and whenever the stream timestamp goes
//  backward (e.g., because of a backward seek).
#define RTMP_FMT_TYPE0 0
// Chunks of Type 1 are 7 bytes long. The message stream ID is not
//  included; this chunk takes the same stream ID as the preceding chunk.
//  Streams with variable-sized messages (for example, many video
//  formats) SHOULD use this format for the first chunk of each new
//  message after the first.
#define RTMP_FMT_TYPE1 1
// Chunks of Type 2 are 3 bytes long. Neither the stream ID nor the
//  message length is included; this chunk has the same stream ID and
//  message length as the preceding chunk. Streams with constant-sized
//  messages (for example, some audio and data formats) SHOULD use this
//  format for the first chunk of each message after the first.
#define RTMP_FMT_TYPE2 0xF2
// Chunks of Type 3 have no header. Stream ID, message length and
//  timestamp delta are not present; chunks of this type take values from
//  the preceding chunk. When a single message is split into chunks, all
//  chunks of a message except the first one, SHOULD use this type. Refer
//  to example 2 in section 6.2.2. Stream consisting of messages of
//  exactly the same size, stream ID and spacing in time SHOULD use this
//  type for all chunks after chunk of Type 2. Refer to example 1 in
//  section 6.2.1. If the delta between the first message and the second
//  message is same as the time stamp of first message, then chunk of
//  type 3 would immediately follow the chunk of type 0 as there is no
//  need for a chunk of type 2 to register the delta. If Type 3 chunk
//  follows a Type 0 chunk, then timestamp delta for this Type 3 chunk is
//  the same as the timestamp of Type 0 chunk.
#define RTMP_FMT_TYPE3 0xF3

/** 
 * @code
 *   1-bytes basic header,
 *   11-bytes message header,
 *   4-bytes timestamp header,
 * @endcode
 */ 
#define RTMP_MAX_FMT0_SZIE 15

/**
 * @code
 *   1-bytes basic header,
 *   4-bytes timestamp header,
 * @endcode
 */ 
#define RTMP_MAX_FMT3_SZIE 5

#define RTMP_DEFAULT_CHUNK_SIZE 128
#define RTMP_MIN_CHUNK_SIZE 2

// AMF0 command message, command name macros
#define RTMP_AMF0_COMMAND_CONNECT           "connect"
#define RTMP_AMF0_COMMAND_CREATE_STREAM     "createStream"
#define RTMP_AMF0_COMMAND_PLAY              "play"
#define RTMP_AMF0_COMMAND_ON_BW_DONE        "onBWDone"
#define RTMP_AMF0_COMMAND_ON_STATUS         "onStatus"
#define RTMP_AMF0_COMMAND_RESULT            "_result"
#define RTMP_AMF0_COMMAND_RELEASE_STREAM    "releaseStream"
#define RTMP_AMF0_COMMAND_FC_PUBLISH        "FCPublish"
#define RTMP_AMF0_COMMAND_UNPUBLISH         "FCUnpublish"
#define RTMP_AMF0_COMMAND_PUBLISH           "publish"
#define RTMP_AMF0_DATA_SAMPLE_ACCESS        "|RtmpSampleAccess"
#define RTMP_AMF0_DATA_SET_DATAFRAME        "@setDataFrame"
#define RTMP_AMF0_DATA_ON_METADATA          "onMetaData"

namespace hare {
namespace core {

    class HARE_API protocol_rtmp : public protocol {
    public:
        enum MESSAGE_TYPE : int8_t {
            /// RTMP reserves message type IDs 1-7 for protocol control messages.
            /// These messages contain information needed by the RTM Chunk Stream
            /// protocol or RTMP itself. Protocol messages with IDs 1 & 2 are
            /// reserved for usage with RTM Chunk Stream protocol. Protocol messages
            /// with IDs 3-6 are reserved for usage of RTMP. Protocol message with ID
            /// 7 is used between edge server and origin server.
            SET_CHUNK_SIZE = 0x01,
#define RTMP_MT_SET_CHUNK_SIZE ProtocolRTMP::SET_CHUNK_SIZE
            ABORT_MESSAGE = 0x02,
#define RTMP_MT_ABORT_MESSAGE ProtocolRTMP::ABORT_MESSAGE
            ACKNOWLEDGEMENT = 0x03,
#define RTMP_MT_ACKNOWLEDGEMENT ProtocolRTMP::ACKNOWLEDGEMENT
            USER_CONTROL_MESSAGE = 0x04,
#define RTMP_MT_USER_CONTROL_MESSAGE ProtocolRTMP::USER_CONTROL_MESSAGE
            WINDOW_ACKNOWLEDGEMENT_SIZE = 0x05,
#define RTMP_MT_WINDOW_ACKNOWLEDGEMENT_SIZE ProtocolRTMP::WINDOW_ACKNOWLEDGEMENT_SIZE
            SET_PEER_BAND_WIDTH = 0x06,
#define RTMP_MT_SET_PEER_BAND_WIDTH ProtocolRTMP::SET_PEER_BAND_WIDTH
            EDGE_AND_ORIGIN_SERVER_COMMAND = 0x07,
#define RTMP_MT_EDGE_AND_ORIGIN_SERVER_COMMAND ProtocolRTMP::EDGE_AND_ORIGIN_SERVER_COMMAND
            /// The client or the server sends this message to send audio data to the
            /// peer. The message type value of 8 is reserved for audio messages.
            AUDIO_MESSAGE = 0x08,
#define RTMP_MT_AUDIO_MESSAGE ProtocolRTMP::AUDIO_MESSAGE
            /// The client or the server sends this message to send video data to the
            /// peer. The message type value of 9 is reserved for video messages.
            /// These messages are large and can delay the sending of other type of
            /// messages. To avoid such a situation, the video message is assigned
            /// the lowest priority.
            VIDEO_MESSAGE = 0x09,
#define RTMP_MT_VIDEO_MESSAGE ProtocolRTMP::VIDEO_MESSAGE
            /// The client or the server sends this message to send Metadata or any
            /// user data to the peer. Metadata includes details about the
            /// data(audio, video etc.) like creation time, duration, theme and so
            /// on. These messages have been assigned message type value of 18 for
            /// AMF0 and message type value of 15 for AMF3. 
            AMF3_DATA_MESSAGE = 0x0F,
#define RTMP_MT_AMF3_DATA_MESSAGE ProtocolRTMP::AMF3_DATA_MESSAGE
            AMF0_DATA_MESSAGE = 0x12,
#define RTMP_MT_AMF0_DATA_MESSAGE ProtocolRTMP::AMF0_DATA_MESSAGE
            /// A shared object is a Flash object (a collection of name value pairs)
            /// that are in synchronization across multiple clients, instances, and
            /// so on. The message types kMsgContainer=19 for AMF0 and
            /// kMsgContainerEx=16 for AMF3 are reserved for shared object events.
            /// Each message can contain multiple events.
            AMF3_SHARED_OBJECT = 0x10,
#define RTMP_MT_AMF3_SHARED_OBJECT ProtocolRTMP::AMF3_SHARED_OBJECT
            AMF0_SHARED_OBJECT = 0x13,
#define RTMP_MT_AMF0_SHARED_OBJECT ProtocolRTMP::AMF0_SHARED_OBJECT
            /// Command messages carry the AMF-encoded commands between the client
            /// and the server. These messages have been assigned message type value
            /// of 20 for AMF0 encoding and message type value of 17 for AMF3
            /// encoding. These messages are sent to perform some operations like
            /// connect, createStream, publish, play, pause on the peer. Command
            /// messages like onstatus, result etc. are used to inform the sender
            /// about the status of the requested commands. A command message
            /// consists of command name, transaction ID, and command object that
            /// contains related parameters. A client or a server can request Remote
            /// Procedure Calls (RPC) over streams that are communicated using the
            /// command messages to the peer.
            AMF3_COMMAND_MESSAGE = 0x11,
#define RTMP_MT_AMF3_COMMAND_MESSAGE ProtocolRTMP::AMF3_COMMAND_MESSAGE
            AMF0_COMMAND_MESSAGE = 0x14,
#define RTMP_MT_AMF0_COMMAND_MESSAGE ProtocolRTMP::AMF0_COMMAND_MESSAGE
            /// An aggregate message is a single message that contains a list of submessages.
            /// The message type value of 22 is reserved for aggregate
            /// messages.
            AGGREGATE_MESSAGE = 0x16
#define RTMP_MT_AGGREGATE_MESSAGE ProtocolRTMP::AGGREGATE_MESSAGE
        };

        struct MessageHeader {
            /**
             * @brief Three-byte field that contains a timestamp delta of the message.
             *   The 4 bytes are packed in the big-endian order.
             *
             * @remark only used for decoding message from chunk stream.
             */
            int32_t timestamp_delta;

            /**
             * @brief Three-byte field that represents the size of the payload in bytes.
             *   It is set in big-endian format.
             */
            int32_t payload_length;

            /**
             * @brief One-byte field to represent the message type. A range of type IDs
             *   (1-7) are reserved for protocol control messages.
             */
            MESSAGE_TYPE message_type;
            
            /**
             * @brief Three-byte field that identifies the stream of the message. These
             *   bytes are set in big-endian format.
             */
            int32_t stream_id;
            
            /**
             * @brief Four-byte field that contains a timestamp of the message.
             *   The 4 bytes are packed in the big-endian order.
             *
             * @remark used as calc timestamp when decode and encode time.
             */
            int32_t extended_timestamp;
        };

    private:
        handshake::ptr hand_shark_ { nullptr };

    public:
        protocol_rtmp();
        ~protocol_rtmp() override = default;

        auto parse(io::buffer& buffer, hare::ptr<net::session> session) -> error override;

    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_PROTOCOL_RTMP_H_