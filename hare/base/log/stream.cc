#include <hare/base/log/stream.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <sstream>

namespace hare {
namespace log {

    namespace detail {

        static const auto dec { 10 };
        static const auto hex { 16 };
        static const std::array<char, dec> sc_digits = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
        static_assert(sc_digits.size() == static_cast<size_t>(dec), "wrong number of digits");

        static const std::array<char, hex + 1> sc_digits_hex = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        static_assert(sc_digits_hex.size() == hex + 1, "wrong number of digits_hex");

        template <typename T>
        auto convert(char* _buf, T _value) -> size_t
        {
            auto value = _value;
            auto* ptr = _buf;

            do {
                auto lsd = static_cast<int32_t>(value % dec);
                value /= dec;
                *ptr++ = sc_digits[lsd];
            } while (value != 0);

            if (value < 0) {
                *ptr++ = '-';
            }
            *ptr = '\0';
            std::reverse(_buf, ptr);

            return ptr - _buf;
        }

        auto convert_hex(char* _buf, uintptr_t _value) -> size_t
        {
            auto value = _value;
            auto* ptr = _buf;

            do {
                auto lsd = static_cast<int32_t>(value % hex);
                value /= hex;
                *ptr++ = sc_digits_hex[lsd];
            } while (value != 0);

            *ptr = '\0';
            std::reverse(_buf, ptr);

            return ptr - _buf;
        }

        template <typename Type>
        fmt::fmt(const char* _fmt, Type _val)
        {
            static_assert(std::is_arithmetic<Type>::value, "Must be arithmetic type");

            length_ = ::snprintf(buf_.data(), buf_.size(), _fmt, _val);
            assert(static_cast<size_t>(length_) < buf_.size());
        }

        // Explicit instantiations
        template fmt::fmt(const char* fmt, char);
        template fmt::fmt(const char* fmt, int16_t);
        template fmt::fmt(const char* fmt, uint16_t);
        template fmt::fmt(const char* fmt, int32_t);
        template fmt::fmt(const char* fmt, uint32_t);
        template fmt::fmt(const char* fmt, int64_t);
        template fmt::fmt(const char* fmt, uint64_t);
        template fmt::fmt(const char* fmt, float);
        template fmt::fmt(const char* fmt, double);

    } // namespace detail

    void stream::static_check()
    {
        static const auto s_log_number_max_len { 10 };
        static_assert(MAX_NUMERSIC_SIZE - s_log_number_max_len > std::numeric_limits<double>::digits10,
            "MAX_NUMERSIC_SIZE is large enough");
        static_assert(MAX_NUMERSIC_SIZE - s_log_number_max_len > std::numeric_limits<long double>::digits10,
            "MAX_NUMERSIC_SIZE is large enough");
        static_assert(MAX_NUMERSIC_SIZE - s_log_number_max_len > std::numeric_limits<int64_t>::digits10,
            "MAX_NUMERSIC_SIZE is large enough");
        static_assert(MAX_NUMERSIC_SIZE - s_log_number_max_len > std::numeric_limits<int64_t>::digits10,
            "MAX_NUMERSIC_SIZE is large enough");
    }

    template <typename Type>
    void stream::format_integer(Type _num)
    {
        if (cache_.avail() >= MAX_NUMERSIC_SIZE) {
            auto len = detail::convert(cache_.current(), _num);
            cache_.skip(len);
        }
    }

    auto stream::operator<<(int16_t _num) -> stream&
    {
        *this << static_cast<int32_t>(_num);
        return *this;
    }

    auto stream::operator<<(uint16_t _num) -> stream&
    {
        *this << static_cast<uint32_t>(_num);
        return *this;
    }

    auto stream::operator<<(int32_t _num) -> stream&
    {
        format_integer(_num);
        return *this;
    }

    auto stream::operator<<(uint32_t _num) -> stream&
    {
        format_integer(_num);
        return *this;
    }

    auto stream::operator<<(int64_t _num) -> stream&
    {
        format_integer(_num);
        return *this;
    }

    auto stream::operator<<(uint64_t _num) -> stream&
    {
        format_integer(_num);
        return *this;
    }

    auto stream::operator<<(const void* _pointer) -> stream&
    {
        auto intptr = reinterpret_cast<uintptr_t>(_pointer);
        if (cache_.avail() >= MAX_NUMERSIC_SIZE) {
            auto* buf = cache_.current();
            buf[0] = '0';
            buf[1] = 'x';
            auto len = detail::convert_hex(buf + 2, intptr);
            cache_.skip(len + 2);
        }
        return *this;
    }

    auto stream::operator<<(double _num) -> stream&
    {
        if (cache_.avail() >= MAX_NUMERSIC_SIZE) {
            /// FIXME: replace this with Grisu3 by Florian Loitsch.
            auto len = ::snprintf(cache_.current(), MAX_NUMERSIC_SIZE, "%.12g", _num);
            cache_.skip(len);
        }
        return *this;
    }

} // namespace log
} // namespace hare