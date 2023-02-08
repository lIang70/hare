#ifndef _HARE_BASE_LOG_UTIL_H_
#define _HARE_BASE_LOG_UTIL_H_

#include <hare/base/util.h>

#include <string>

namespace hare {
namespace log {

    static const int32_t SMALL_BUFFER = 4 * 1024;
    static const int32_t LARGE_BUFFER = 1024 * SMALL_BUFFER;

    namespace detail {

        template <int SIZE>
        class FixedBuffer {
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

            FixedBuffer(const FixedBuffer&) = delete;
            FixedBuffer& operator=(const FixedBuffer&) = delete;

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

        class Fmt {
            char buf_[32] {};
            int length_ { 0 };

        public:
            template <typename T>
            Fmt(const char* fmt, T val);

            const char* data() const { return buf_; }
            int length() const { return length_; }
        };

        // Format quantity n in SI units (k, M, G, T, P, E).
        // The returned string is atmost 5 characters long.
        // Requires n >= 0
        extern std::string formatSI(int64_t n);

        // Format quantity n in IEC (binary) units (Ki, Mi, Gi, Ti, Pi, Ei).
        // The returned string is atmost 6 characters long.
        // Requires n >= 0
        extern std::string formatIEC(int64_t n);

    } // namespace detail

} // namespace log
} // namespace hare

#endif // !_HARE_BASE_LOG_UTIL_H_