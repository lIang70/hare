/**
 * @file hare/base/log/stream.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with stream.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_LOG_STREAM_H_
#define _HARE_BASE_LOG_STREAM_H_

#include <hare/base/util/buffer.h>

#include <string>

namespace hare {
namespace log {

    namespace detail {

        class HARE_API fmt {
            std::array<char, HARE_SMALL_FIXED_SIZE> buf_ {};
            int32_t length_ { 0 };

        public:
            template <typename Type>
            fmt(const char* _fmt, Type _val);

            inline auto data() const -> const char* { return buf_.data(); }
            inline auto length() const -> int32_t { return length_; }
        };
    } // namespace detail

    class HARE_API stream {
    public:
        using fixed_buffer = util::fixed_buffer<HARE_SMALL_BUFFER>;
        using format = detail::fmt;

        static const int32_t MAX_NUMERSIC_SIZE = 48;

    private:
        fixed_buffer buffer_;

    public:
        auto operator<<(bool _check) -> stream&
        {
            *this << (_check ? "true" : "false");
            return *this;
        }

        auto operator<<(int16_t) -> stream&;
        auto operator<<(uint16_t) -> stream&;
        auto operator<<(int32_t) -> stream&;
        auto operator<<(uint32_t) -> stream&;
        auto operator<<(int64_t) -> stream&;
        auto operator<<(uint64_t) -> stream&;

        auto operator<<(const void*) -> stream&;

        auto operator<<(float _num) -> stream&
        {
            *this << static_cast<double>(_num);
            return *this;
        }
        auto operator<<(double) -> stream&;

        auto operator<<(char _ch) -> stream&
        {
            buffer_.append(&_ch, 1);
            return *this;
        }

        auto operator<<(const char* _str) -> stream&
        {
            const auto null_size = 6;
            if (_str != nullptr) {
                buffer_.append(_str, strlen(_str));
            } else {
                buffer_.append("(null)", null_size);
            }
            return *this;
        }

        auto operator<<(const unsigned char* _str) -> stream&
        {
            return operator<<(reinterpret_cast<const char*>(_str));
        }

        auto operator<<(const std::string& _str) -> stream&
        {
            buffer_.append(_str.c_str(), _str.size());
            return *this;
        }

        auto operator<<(const fixed_buffer& _buffer) -> stream&
        {
            *this << _buffer.to_string();
            return *this;
        }

        void append(const char* _data, size_t _len) { buffer_.append(_data, _len); }
        auto buffer() const -> const fixed_buffer& { return buffer_; }
        void reset() { buffer_.reset(); }

    private:
        static void static_check();

        template <typename T>
        void format_integer(T);
    };

    inline auto operator<<(stream& _stream, const detail::fmt& _fmt) -> stream&
    {
        _stream.append(_fmt.data(), _fmt.length());
        return _stream;
    }

} // namespace log
} // namespace hare

#endif // !_HARE_BASE_LOG_STREAM_H_