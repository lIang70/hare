/**
 * @file hare/base/format/core.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with core.h
 * @version 0.1-beta
 * @date 2023-07-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_FORMAT_CORE_H_
#define _HARE_FORMAT_CORE_H_

#include <hare/base/fwd.h>

template <typename Char>
class basic_string_view {
    const Char* data_;
    size_t size_;

public:
    using value_type = Char;
    using iterator = const Char*;

    constexpr basic_string_view() noexcept
        : data_(nullptr)
        , size_(0)
    {
    }

    /** Constructs a string reference object from a C string and a size. */
    constexpr basic_string_view(const Char* s, size_t count) noexcept
        : data_(s)
        , size_(count)
    {
    }

    /**
     * Constructs a string reference object from a C string computing
     * the size with ``std::char_traits<Char>::length``.
     */
    FMT_CONSTEXPR_CHAR_TRAITS
    FMT_INLINE
    basic_string_view(const Char* s)
        : data_(s)
        , size_(detail::const_check(std::is_same<Char, char>::value && !detail::is_constant_evaluated(true))
                  ? std::strlen(reinterpret_cast<const char*>(s))
                  : std::char_traits<Char>::length(s))
    {
    }

    /** Constructs a string reference from a ``std::basic_string`` object. */
    template <typename Traits, typename Alloc>
    FMT_CONSTEXPR basic_string_view(
        const std::basic_string<Char, Traits, Alloc>& s) noexcept
        : data_(s.data())
        , size_(s.size())
    {
    }

    template <typename S, FMT_ENABLE_IF(std::is_same<S, detail::std_string_view<Char>>::value)>
    FMT_CONSTEXPR basic_string_view(S s) noexcept
        : data_(s.data())
        , size_(s.size())
    {
    }

    /** Returns a pointer to the string data. */
    constexpr auto data() const noexcept -> const Char* { return data_; }

    /** Returns the string size. */
    constexpr auto size() const noexcept -> size_t { return size_; }

    constexpr auto begin() const noexcept -> iterator { return data_; }
    constexpr auto end() const noexcept -> iterator { return data_ + size_; }

    constexpr auto operator[](size_t pos) const noexcept -> const Char&
    {
        return data_[pos];
    }

    FMT_CONSTEXPR void remove_prefix(size_t n) noexcept
    {
        data_ += n;
        size_ -= n;
    }

    // Lexicographically compare this string reference to other.
    FMT_CONSTEXPR_CHAR_TRAITS auto compare(basic_string_view other) const -> int
    {
        size_t str_size = size_ < other.size_ ? size_ : other.size_;
        int result = std::char_traits<Char>::compare(data_, other.data_, str_size);
        if (result == 0)
            result = size_ == other.size_ ? 0 : (size_ < other.size_ ? -1 : 1);
        return result;
    }

    FMT_CONSTEXPR_CHAR_TRAITS friend auto operator==(basic_string_view lhs,
        basic_string_view rhs)
        -> bool
    {
        return lhs.compare(rhs) == 0;
    }
    friend auto operator!=(basic_string_view lhs, basic_string_view rhs) -> bool
    {
        return lhs.compare(rhs) != 0;
    }
    friend auto operator<(basic_string_view lhs, basic_string_view rhs) -> bool
    {
        return lhs.compare(rhs) < 0;
    }
    friend auto operator<=(basic_string_view lhs, basic_string_view rhs) -> bool
    {
        return lhs.compare(rhs) <= 0;
    }
    friend auto operator>(basic_string_view lhs, basic_string_view rhs) -> bool
    {
        return lhs.compare(rhs) > 0;
    }
    friend auto operator>=(basic_string_view lhs, basic_string_view rhs) -> bool
    {
        return lhs.compare(rhs) >= 0;
    }
};

using string_view = basic_string_view<char>;

#endif // _HARE_FORMAT_CORE_H_