#ifndef _HARE_CORE_PROTOCOL_H_
#define _HARE_CORE_PROTOCOL_H_

#include <hare/base/error.h>
#include <hare/base/io/buffer.h>

#define BITS_PER_BYTE 8
#define ONE_KILO 1024

namespace hare {
namespace net {
    class session;
} // namespace net

namespace core {

    using PROTOCOL_TYPE = enum {
        PROTOCOL_TYPE_INVALID,
        PROTOCOL_TYPE_RTMP
    };

    class HARE_API protocol {
        PROTOCOL_TYPE type_ { PROTOCOL_TYPE_INVALID };

    public:
        explicit protocol(PROTOCOL_TYPE _type)
            : type_(_type)
        {
        }
        virtual ~protocol() = default;

        inline auto type() -> PROTOCOL_TYPE { return type_; }

        virtual auto parse(io::buffer& buffer, ptr<net::session> session) -> error = 0;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_PROTOCOL_H_