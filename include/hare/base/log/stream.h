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
 */

#ifndef _HARE_BASE_LOG_STREAM_H_
#define _HARE_BASE_LOG_STREAM_H_

#include <hare/base/util/non_copyable.h>

#include <string>
#include <thread>

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
            using Ptr = std::shared_ptr<FixedBuffer<SIZE>>;

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

            auto begin() -> char* { return data_; }
            auto data() const -> const char* { return data_; }
            auto length() const -> int { return static_cast<int>(cur_ - data_); }

            // write to data_ directly
            auto current() -> char* { return cur_; }
            auto avail() const -> int { return static_cast<int>(end() - cur_); }
            void add(size_t len) { cur_ += len; }

            void reset() { cur_ = data_; }
            void bzero() { setZero(data_, sizeof(data_)); }

            void setCookie(void (*cookie)()) { cookie_ = cookie; }

            auto toString() const -> std::string { return std::string(data_, length()); }

        private:
            auto end() const -> const char* { return data_ + sizeof(data_); }
            static void cookieStart() { }
            static void cookieEnd() { }
        };

        class HARE_API Fmt {
            char buf_[HARE_SMALL_FIXED_SIZE] {};
            int32_t length_ { 0 };

        public:
            template <typename T>
            Fmt(const char* fmt, T val);

            inline auto data() const -> const char* { return buf_; }
            inline auto length() const -> int32_t { return length_; }
        };
    } // namespace detail

    class HARE_API Stream {
    public:
        using Buffer = detail::FixedBuffer<SMALL_BUFFER>;

        static const int32_t MAX_NUMERSIC_SIZE = 48;

    private:
        Buffer buffer_;

    public:
        auto operator<<(bool boo) -> Stream&
        {
            buffer_.append(boo ? "1" : "0", 1);
            return *this;
        }

        auto operator<<(int16_t) -> Stream&;
        auto operator<<(uint16_t) -> Stream&;
        auto operator<<(int32_t) -> Stream&;
        auto operator<<(uint32_t) -> Stream&;
        auto operator<<(int64_t) -> Stream&;
        auto operator<<(uint64_t) -> Stream&;

        auto operator<<(const void*) -> Stream&;

        auto operator<<(float num) -> Stream&
        {
            *this << static_cast<double>(num);
            return *this;
        }
        auto operator<<(double) -> Stream&;

        auto operator<<(char one_ch) -> Stream&
        {
            buffer_.append(&one_ch, 1);
            return *this;
        }

        auto operator<<(const char* str) -> Stream&
        {
            const auto null_size = 6;
            if (str != nullptr) {
                buffer_.append(str, strlen(str));
            } else {
                buffer_.append("(null)", null_size);
            }
            return *this;
        }

        auto operator<<(const unsigned char* str) -> Stream&
        {
            return operator<<(reinterpret_cast<const char*>(str));
        }

        auto operator<<(const std::string& str) -> Stream&
        {
            buffer_.append(str.c_str(), str.size());
            return *this;
        }

        auto operator<<(const Buffer& buffer) -> Stream&
        {
            *this << buffer.toString();
            return *this;
        }

        auto operator<<(std::thread::id tid) -> Stream&;

        void append(const char* data, int len) { buffer_.append(data, len); }
        auto buffer() const -> const Buffer& { return buffer_; }
        void resetBuffer() { buffer_.reset(); }

    private:
        static void staticCheck();

        template <typename T>
        void formatInteger(T);
    };

    inline auto operator<<(Stream& stream, const detail::Fmt& fmt) -> Stream&
    {
        stream.append(fmt.data(), fmt.length());
        return stream;
    }

} // namespace log
} // namespace hare

#endif // !_HARE_BASE_LOG_STREAM_H_