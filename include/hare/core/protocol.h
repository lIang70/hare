#ifndef _HARE_CORE_PROTOCOL_H_
#define _HARE_CORE_PROTOCOL_H_

#include <hare/base/error.h>
#include <hare/net/buffer.h>
#include <hare/core/stream_session.h>

#define BITS_PER_BYTE   8
#define ONE_KILO        1024

namespace hare {
namespace core {

    class HARE_API Protocol {
    public:
        using Ptr = std::shared_ptr<Protocol>;

        Protocol() = default;
        virtual ~Protocol() = default;

        virtual auto parse(net::Buffer& buffer, StreamSession::Ptr session) -> Error = 0;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_PROTOCOL_H_