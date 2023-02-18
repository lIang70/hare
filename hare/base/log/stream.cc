#include <hare/base/detail/log_stream.h>

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
        std::size_t convert(char buf[], T value)
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

        std::size_t convertHex(char buf[], uintptr_t value)
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
            static_assert(std::is_arithmetic<T>::value == true, "Must be arithmetic type");

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
    void Stream::formatInteger(T v)
    {
        if (buffer_.avail() >= MAX_NUMERSIC_SIZE) {
            auto len = detail::convert(buffer_.current(), v);
            buffer_.add(len);
        }
    }

    Stream& Stream::operator<<(int16_t v)
    {
        *this << static_cast<int32_t>(v);
        return *this;
    }

    Stream& Stream::operator<<(uint16_t v)
    {
        *this << static_cast<uint32_t>(v);
        return *this;
    }

    Stream& Stream::operator<<(int32_t v)
    {
        formatInteger(v);
        return *this;
    }

    Stream& Stream::operator<<(uint32_t v)
    {
        formatInteger(v);
        return *this;
    }

    Stream& Stream::operator<<(int64_t v)
    {
        formatInteger(v);
        return *this;
    }

    Stream& Stream::operator<<(uint64_t v)
    {
        formatInteger(v);
        return *this;
    }

    Stream& Stream::operator<<(const void* p)
    {
        auto v = reinterpret_cast<uintptr_t>(p);
        if (buffer_.avail() >= MAX_NUMERSIC_SIZE) {
            auto buf = buffer_.current();
            buf[0] = '0';
            buf[1] = 'x';
            auto len = detail::convertHex(buf + 2, v);
            buffer_.add(len + 2);
        }
        return *this;
    }

    // FIXME: replace this with Grisu3 by Florian Loitsch.
    Stream& Stream::operator<<(double v)
    {
        if (buffer_.avail() >= MAX_NUMERSIC_SIZE) {
            auto len = snprintf(buffer_.current(), MAX_NUMERSIC_SIZE, "%.12g", v);
            buffer_.add(len);
        }
        return *this;
    }

}
}