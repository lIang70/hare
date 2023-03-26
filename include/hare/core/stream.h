#ifndef _HARE_CORE_STREAM_H_
#define _HARE_CORE_STREAM_H_

#include <cinttypes>

namespace hare {
namespace core {

    class Stream {
        char* data_ { nullptr };
        char* cur_ { nullptr };
        std::size_t size_ { 0 };
    
    public:
        Stream() = default;
        virtual ~Stream() = default;

        inline void setReset() { cur_ = data_; }
        inline auto empty() const -> bool { return (cur_ == nullptr) || (data_ == nullptr) || (cur_ >= data_ + size_); }
        inline auto require(std::size_t required_size) -> bool { return !empty() && (required_size <= data_ + size_ - cur_); }
        inline void skip(std::size_t size) { cur_ += size; }
        inline auto length() const -> std::size_t { return empty() ? 0 : static_cast<std::size_t>(cur_ - data_); }

        virtual auto setBuffer(char* data, std::size_t size) -> int32_t;

        auto readOneByte() -> int8_t;
        auto readTwoBytes() -> int16_t;
        auto readFourBytes() -> int32_t;
        auto readEightBytes() -> int32_t;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_STREAM_H_