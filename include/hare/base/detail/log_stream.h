#ifndef _HARE_BASE_LOG_STREAM_H_
#define _HARE_BASE_LOG_STREAM_H_

#include <hare/base/detail/log_util.h>

namespace hare {
namespace log {

    class Stream {
    public:
        using Buffer = detail::FixedBuffer<SMALL_BUFFER>;

        static const int32_t MAX_NUMERSIC_SIZE = 48;

    private:
        Buffer buffer_;

    public:
        Stream& operator<<(bool v)
        {
            buffer_.append(v ? "1" : "0", 1);
            return *this;
        }

        Stream& operator<<(short);
        Stream& operator<<(unsigned short);
        Stream& operator<<(int);
        Stream& operator<<(unsigned int);
        Stream& operator<<(long);
        Stream& operator<<(unsigned long);
        Stream& operator<<(long long);
        Stream& operator<<(unsigned long long);

        Stream& operator<<(const void*);

        Stream& operator<<(float v)
        {
            *this << static_cast<double>(v);
            return *this;
        }
        Stream& operator<<(double);

        Stream& operator<<(char v)
        {
            buffer_.append(&v, 1);
            return *this;
        }

        Stream& operator<<(const char* str)
        {
            if (str) {
                buffer_.append(str, strlen(str));
            } else {
                buffer_.append("(null)", 6);
            }
            return *this;
        }

        Stream& operator<<(const unsigned char* str)
        {
            return operator<<(reinterpret_cast<const char*>(str));
        }

        Stream& operator<<(const std::string& v)
        {
            buffer_.append(v.c_str(), v.size());
            return *this;
        }

        Stream& operator<<(const Buffer& v)
        {
            *this << v.toString();
            return *this;
        }

        void append(const char* data, int len) { buffer_.append(data, len); }
        const Buffer& buffer() const { return buffer_; }
        void resetBuffer() { buffer_.reset(); }

    private:
        void staticCheck();

        template <typename T>
        void formatInteger(T);
    };

    inline Stream& operator<<(Stream& s, const detail::Fmt& fmt)
    {
        s.append(fmt.data(), fmt.length());
        return s;
    }

} // namespace log
} // namespace hare

#endif // !_HARE_BASE_LOG_STREAM_H_