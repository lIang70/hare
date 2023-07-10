/**
 * @file hare/base/util/buffer.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with buffer.h
 * @version 0.1-beta
 * @date 2023-07-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_UTIL_BUFFER_H_
#define _HARE_BASE_UTIL_BUFFER_H_

#include <hare/base/fwd.h>

#include <cassert>

namespace hare {
namespace util {

    namespace detail {
        template <typename T>
        using checked_ptr = T*;

        template <typename T>
        constexpr auto make_checked(T* p, std::size_t) -> T*
        {
            return p;
        }

        template <typename Int>
        inline auto to_unsigned(Int value) ->
            typename std::make_unsigned<Int>::type
        {
            assert(std::is_unsigned<Int>::value || value >= 0);
            return static_cast<typename std::make_unsigned<Int>::type>(value);
        }
    } // namespace detail

    template <typename T>
    class buffer {
        T* ptr_ {};
        std::size_t size_ {};
        std::size_t capacity_ {};

    protected:
        // Don't initialize ptr_ since it is not accessed to save a few cycles.
        explicit buffer(std::size_t sz) noexcept
            : size_(sz)
            , capacity_(sz)
        {
        }

        explicit buffer(T* p = nullptr, std::size_t sz = 0, std::size_t cap = 0) noexcept
            : ptr_(p)
            , size_(sz)
            , capacity_(cap)
        {
        }

        buffer(buffer&&) noexcept = default;

        /** Sets the buffer data and capacity. */
        inline void set(T* buf_data, std::size_t buf_capacity) noexcept
        {
            ptr_ = buf_data;
            capacity_ = buf_capacity;
        }

        /** Increases the buffer capacity to hold at least *capacity* elements. */
        virtual void grow(std::size_t capacity) = 0;

    public:
        using value_type = T;
        using const_reference = const T&;

        virtual ~buffer() = default;

        buffer(const buffer&) = delete;
        void operator=(const buffer&) = delete;

        inline auto begin() noexcept -> T* { return ptr_; }
        inline auto end() noexcept -> T* { return ptr_ + size_; }

        inline auto begin() const noexcept -> const T* { return ptr_; }
        inline auto end() const noexcept -> const T* { return ptr_ + size_; }

        /** Returns the size of this buffer. */
        constexpr auto size() const noexcept -> std::size_t { return size_; }

        /** Returns the capacity of this buffer. */
        constexpr auto capacity() const noexcept -> std::size_t { return capacity_; }

        /** Returns a pointer to the buffer data. */
        auto data() noexcept -> T* { return ptr_; }

        /** Returns a pointer to the buffer data. */
        constexpr auto data() const noexcept -> const T* { return ptr_; }

        /** Clears this buffer. */
        inline void clear() { size_ = 0; }

        inline void skip(std::size_t size)
        {
            if (size_ + size > capacity_) {
                size_ = capacity_;
            } else {
                size_ += size;
            }
        }

        // Tries resizing the buffer to contain *count* elements. If T is a POD type
        // the new elements may not be initialized.
        void try_resize(std::size_t count)
        {
            try_reserve(count);
            size_ = count <= capacity_ ? count : capacity_;
        }

        // Tries increasing the buffer capacity to *new_capacity*. It can increase the
        // capacity by a smaller amount than requested but guarantees there is space
        // for at least one additional element either by increasing the capacity or by
        // flushing the buffer if it is full.
        void try_reserve(std::size_t new_capacity)
        {
            if (new_capacity > capacity_) {
                grow(new_capacity);
            }
        }

        void push_back(const T& value)
        {
            try_reserve(size_ + 1);
            ptr_[size_++] = value;
        }

        /** Appends data to the end of the buffer. */
        template <typename U>
        void append(const U* begin, const U* end);

        template <typename Idx>
        auto operator[](Idx index) -> T&
        {
            return ptr_[index];
        }

        template <typename Idx>
        constexpr auto operator[](Idx index) const -> const T&
        {
            return ptr_[index];
        }
    };

    struct buffer_traits {
        explicit buffer_traits(std::size_t) { }
        auto count() const -> std::size_t { return 0; }
        auto limit(std::size_t size) -> std::size_t { return size; }
    };

    class fixed_buffer_traits {
        std::size_t count_ = 0;
        std::size_t limit_;

    public:
        explicit fixed_buffer_traits(std::size_t limit)
            : limit_(limit)
        {
        }
        auto count() const -> std::size_t { return count_; }
        auto limit(std::size_t size) -> std::size_t
        {
            auto n = limit_ > count_ ? limit_ - count_ : 0;
            count_ += size;
            return size < n ? size : n;
        }
    };

    template <typename T>
    template <typename U>
    void buffer<T>::append(const U* begin, const U* end)
    {
        while (begin != end) {
            auto count = detail::to_unsigned(end - begin);
            try_reserve(size_ + count);
            auto free_cap = capacity_ - size_;
            if (free_cap < count) {
                count = free_cap;
            }
            std::uninitialized_copy_n(begin, count, detail::make_checked(ptr_ + size_, count));
            size_ += count;
            begin += count;
        }
    }

} // namespace util
} // namespace hare

#endif // _HARE_BASE_UTIL_BUFFER_H_