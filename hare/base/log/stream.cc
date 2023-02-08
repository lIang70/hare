#include <hare/base/detail/log_stream.h>

#include <algorithm>
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
            T i = value;
            char* p = buf;

            do {
                int32_t lsd = static_cast<int32_t>(i % 10);
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
            uintptr_t i = value;
            char* p = buf;

            do {
                int32_t lsd = static_cast<int32_t>(i % 16);
                i /= 16;
                *p++ = c_digits_hex[lsd];
            } while (i != 0);

            *p = '\0';
            std::reverse(buf, p);

            return p - buf;
        }

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
            std::size_t len = detail::convert(buffer_.current(), v);
            buffer_.add(len);
        }
    }

    Stream& Stream::operator<<(short v)
    {
        *this << static_cast<int>(v);
        return *this;
    }

    Stream& Stream::operator<<(unsigned short v)
    {
        *this << static_cast<unsigned int>(v);
        return *this;
    }

    Stream& Stream::operator<<(int v)
    {
        formatInteger(v);
        return *this;
    }

    Stream& Stream::operator<<(unsigned int v)
    {
        formatInteger(v);
        return *this;
    }

    Stream& Stream::operator<<(long v)
    {
        formatInteger(v);
        return *this;
    }

    Stream& Stream::operator<<(unsigned long v)
    {
        formatInteger(v);
        return *this;
    }

    Stream& Stream::operator<<(long long v)
    {
        formatInteger(v);
        return *this;
    }

    Stream& Stream::operator<<(unsigned long long v)
    {
        formatInteger(v);
        return *this;
    }

    Stream& Stream::operator<<(const void* p)
    {
        uintptr_t v = reinterpret_cast<uintptr_t>(p);
        if (buffer_.avail() >= MAX_NUMERSIC_SIZE) {
            char* buf = buffer_.current();
            buf[0] = '0';
            buf[1] = 'x';
            std::size_t len = detail::convertHex(buf + 2, v);
            buffer_.add(len + 2);
        }
        return *this;
    }

    // FIXME: replace this with Grisu3 by Florian Loitsch.
    Stream& Stream::operator<<(double v)
    {
        if (buffer_.avail() >= MAX_NUMERSIC_SIZE) {
            int len = snprintf(buffer_.current(), MAX_NUMERSIC_SIZE, "%.12g", v);
            buffer_.add(len);
        }
        return *this;
    }

}
}