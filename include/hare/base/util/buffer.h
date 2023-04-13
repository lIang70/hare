#ifndef _HARE_UTIL_BUFFER_H_
#define _HARE_UTIL_BUFFER_H_

#include <hare/base/util/non_copyable.h>

#include <array>
#include <string>

namespace hare {
namespace util {

    template <int32_t Size>
    class HARE_API fixed_buffer : public non_copyable {
        void (*cookie_)() { nullptr };
        std::array<char, Size> data_ {};
        char* cur_ { nullptr };

    public:
        using ptr = ptr<fixed_buffer<Size>>;

        fixed_buffer()
            : cur_(data_.data())
        {
            set_cookie(cookie_start);
        }

        ~fixed_buffer()
        {
            set_cookie(cookie_end);
        }

        void append(const char* _buf, size_t _length)
        {
            if (implicit_cast<size_t>(avail()) > _length) {
                memcpy(cur_, _buf, _length);
                cur_ += _length;
            }
        }

        auto begin() -> char* { return data_.data(); }
        auto data() const -> const char* { return data_.data(); }
        auto length() const -> size_t { return static_cast<size_t>(cur_ - data_.data()); }

        // write to data_ directly
        auto current() -> char* { return cur_; }
        auto avail() const -> size_t { return static_cast<size_t>(end() - cur_); }
        void add(size_t _length) { cur_ += _length; }

        void reset() { cur_ = data_.data(); }
        void bzero() { set_zero(data_.data(), Size); }

        void set_cookie(void (*_cookie)()) { cookie_ = _cookie; }

        auto to_string() const -> std::string { return { data_.data(), length() }; }

    private:
        auto end() const -> const char* { return data_.data() + Size; }
        static void cookie_start() { }
        static void cookie_end() { }
    };

} // namespace util
} // namespace hare

#endif // !_HARE_UTIL_BUFFER_H_