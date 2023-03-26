#include <hare/base/log/stream.h>

#include <algorithm>
#include <cassert>
#include <limits>

namespace hare {
namespace log {

    namespace detail {

        static const char digits[] = "9876543210123456789";
        static const char* zero = digits + 9;
        static_assert(sizeof(digits) == 20, "wrong number of digits");

        static const char c_digits_hex[] = "0123456789ABCDEF";
        static_assert(sizeof(c_digits_hex) == 17, "wrong number of c_digits_hex");

        template <typename T>
        auto convert(char buf[], T value) -> std::size_t
        {
            auto i = value;
            auto p = buf;

            do {
                auto lsd = static_cast<int32_t>(i % 10);
                i /= 10;
                *p++ = zero[lsd];
            } while (i != 0);

            if (value < 0) {
                *p++ = '-';
            }
            *p = '\0';
            std::reverse(buf, p);

            return p - buf;
        }

        auto convertHex(char buf[], uintptr_t value) -> std::size_t
        {
            auto i = value;
            auto p = buf;

            do {
                auto lsd = static_cast<int32_t>(i % 16);
                i /= 16;
                *p++ = c_digits_hex[lsd];
            } while (i != 0);

            *p = '\0';
            std::reverse(buf, p);

            return p - buf;
        }

        template <typename T>
        Fmt::Fmt(const char* fmt, T val)
        {
            static_assert(std::is_arithmetic<T>::value, "Must be arithmetic type");

            length_ = snprintf(buf_, sizeof(buf_), fmt, val);
            assert(static_cast<size_t>(length_) < sizeof(buf_));
        }

        // Explicit instantiations
        template Fmt::Fmt(const char* fmt, char);

        template Fmt::Fmt(const char* fmt, int16_t);
        template Fmt::Fmt(const char* fmt, uint16_t);
        template Fmt::Fmt(const char* fmt, int32_t);
        template Fmt::Fmt(const char* fmt, uint32_t);
        template Fmt::Fmt(const char* fmt, int64_t);
        template Fmt::Fmt(const char* fmt, uint64_t);

        template Fmt::Fmt(const char* fmt, float);
        template Fmt::Fmt(const char* fmt, double);

    } // namespace detail

    void Stream::staticCheck()
    {
        static_assert(MAX_NUMERSIC_SIZE - 10 > std::numeric_limits<double>::digits10,
            "MAX_NUMERSIC_SIZE is large enough");
        static_assert(MAX_NUMERSIC_SIZE - 10 > std::numeric_limits<long double>::digits10,
            "MAX_NUMERSIC_SIZE is large enough");
        static_assert(MAX_NUMERSIC_SIZE - 10 > std::numeric_limits<long>::digits10,
            "MAX_NUMERSIC_SIZE is large enough");
        static_assert(MAX_NUMERSIC_SIZE - 10 > std::numeric_limits<long long>::digits10,
            "MAX_NUMERSIC_SIZE is large enough");
    }

    template <typename T>
    void Stream::formatInteger(T num)
    {
        if (buffer_.avail() >= MAX_NUMERSIC_SIZE) {
            auto len = detail::convert(buffer_.current(), num);
            buffer_.add(len);
        }
    }

    auto Stream::operator<<(int16_t num) -> Stream&
    {
        *this << static_cast<int32_t>(num);
        return *this;
    }

    auto Stream::operator<<(uint16_t num) -> Stream&
    {
        *this << static_cast<uint32_t>(num);
        return *this;
    }

    auto Stream::operator<<(int32_t num) -> Stream&
    {
        formatInteger(num);
        return *this;
    }

    auto Stream::operator<<(uint32_t num) -> Stream&
    {
        formatInteger(num);
        return *this;
    }

    auto Stream::operator<<(int64_t num) -> Stream&
    {
        formatInteger(num);
        return *this;
    }

    auto Stream::operator<<(uint64_t num) -> Stream&
    {
        formatInteger(num);
        return *this;
    }

    auto Stream::operator<<(const void* pointer) -> Stream&
    {
        auto intptr = reinterpret_cast<uintptr_t>(pointer);
        if (buffer_.avail() >= MAX_NUMERSIC_SIZE) {
            auto* buf = buffer_.current();
            buf[0] = '0';
            buf[1] = 'x';
            auto len = detail::convertHex(buf + 2, intptr);
            buffer_.add(len + 2);
        }
        return *this;
    }

    // FIXME: replace this with Grisu3 by Florian Loitsch.
    auto Stream::operator<<(double num) -> Stream&
    {
        if (buffer_.avail() >= MAX_NUMERSIC_SIZE) {
            auto len = snprintf(buffer_.current(), MAX_NUMERSIC_SIZE, "%.12g", num);
            buffer_.add(len);
        }
        return *this;
    }

} // namespace log
} // namespace hare