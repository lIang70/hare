#include <hare/core/stream.h>
#include <hare/base/logging.h>

#include <endian.h>

namespace hare {
namespace core {

    auto Stream::setBuffer(char* data, std::size_t size) -> int32_t
    {
        return 0;
    }

    auto Stream::readOneByte() -> int8_t
    {
        HARE_ASSERT(require(1), "");
        return static_cast<int8_t>(*cur_++);
    }

    auto Stream::readTwoBytes() -> int16_t
    {
        HARE_ASSERT(require(2), "");
        
        return static_cast<int16_t>(*cur_++);
    }

} // namespace core
} // namespace hare