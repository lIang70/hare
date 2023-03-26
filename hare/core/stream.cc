#include <hare/core/stream.h>
#include <hare/base/logging.h>

#include <endian.h>

namespace hare {
namespace core {

    int32_t Stream::setBuffer(char* data, std::size_t size)
    {
        return 0;
    }

    int8_t Stream::readOneByte()
    {
        HARE_ASSERT(require(1), "");
        return (int8_t)*cur_++;
    }

    int16_t Stream::readTwoBytes()
    {
        HARE_ASSERT(require(2), "");
        
        return (int16_t)*cur_++;
    }

} // namespace core
} // namespace hare