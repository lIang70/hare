#ifndef _HARE_BASE_LOG_STREAM_H_
#define _HARE_BASE_LOG_STREAM_H_

#include <hare/base/detail/non_copyable.h>
#include <hare/base/util.h>

namespace hare {
namespace log {
    static const int32_t SMALL_BUFFER = 4 * 1024;
    static const int32_t LARGE_BUFFER = 1024 * SMALL_BUFFER;

    namespace detail {

        template <int SIZE>
        class HARE_API FixedBuffer : public NonCopyable {
            void (*cookie_)() { nullptr };
            char data_[SIZE] {};
            char* cur_ { nullptr };

        public:
            FixedBuffer()
                : cur_(data_)
            {
                setCookie(cookieStart);
            }

            ~FixedBuffer()
            {
                setCookie(cookieEnd);
            }

            void append(const char* buf, size_t len)
            {
                if (implicit_cast<size_t>(avail()) > len) {
                    memcpy(cur_, buf, len);
                    cur_ += len;
                }
            }

            const char* data() const { return data_; }
            int length() const { return static_cast<int>(cur_ - data_); }

            // write to data_ directly
            char* current() { return cur_; }
            int avail() const { return static_cast<int>(end() - cur_); }
            void add(size_t len) { cur_ += len; }

            void reset() { cur_ = data_; }
            void bzero() { setZero(data_, sizeof(data_)); }

            void setCookie(void (*cookie)()) { cookie_ = cookie; }

            std::string toString() const { return std::string(data_, length()); }

        private:
            const char* end() const { return data_ + sizeof(data_); }
            static void cookieStart() { }
            static void cookieEnd() { }
        };

        class HARE_API Fmt {
            char buf_[32] {};
            int32_t length_ { 0 };

        public:
            template <typename T>
            Fmt(const char* fmt, T val);

            inline const char* data() const { return buf_; }
            inline int32_t length() const { return length_; }
        };
    } // namespace detail

    class HARE_API Stream {
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

        Stream& operator<<(int16_t);
        Stream& operator<<(uint16_t);
        Stream& operator<<(int32_t);
        Stream& operator<<(uint32_t);
        Stream& operator<<(int64_t);
        Stream& operator<<(uint64_t);

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
        static void staticCheck();

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