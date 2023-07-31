/**
 * @file hare/net/buffer.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with buffer.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_BUFFER_H_
#define _HARE_NET_BUFFER_H_

#include <hare/base/util/non_copyable.h>

#define HARE_MAX_READ_DEFAULT 4096

namespace hare {
namespace net {

    class buffer;

    HARE_CLASS_API
    class HARE_API buffer_iterator : public util::non_copyable {
        hare::detail::impl* impl_ {};

    public:
        HARE_INLINE
        buffer_iterator(buffer_iterator&& _other) noexcept
        { move(_other); }

        HARE_INLINE
        auto operator=(buffer_iterator&& _other) noexcept -> buffer_iterator&
        { move(_other); return (*this); }

        HARE_INLINE
        auto valid() const -> bool
        { return impl_ != nullptr; }

        auto operator*() noexcept -> char;
        auto operator++() noexcept -> buffer_iterator&;
        auto operator--() noexcept -> buffer_iterator&;

        friend auto operator==(const buffer_iterator& _x, const buffer_iterator& _y) noexcept -> bool;
        friend auto operator!=(const buffer_iterator& _x, const buffer_iterator& _y) noexcept -> bool;

    private:
        HARE_INLINE
        explicit buffer_iterator(hare::detail::impl* _impl)
            : impl_(_impl)
        { }

        HARE_INLINE
        void move(buffer_iterator& _other) noexcept
        { std::swap(impl_, _other.impl_); }

        friend class net::buffer;
    };

    HARE_CLASS_API
    class HARE_API buffer : public util::non_copyable {
        hare::detail::impl* impl_ {};

    public:
        using ptr = ptr<buffer>;
        using iterator = buffer_iterator;

        explicit buffer(std::size_t _max_read = HARE_MAX_READ_DEFAULT);
        ~buffer();

        HARE_INLINE
        buffer(buffer&& _other) noexcept
        { move(_other); }

        HARE_INLINE
        auto operator=(buffer&& _other) noexcept -> buffer&
        { move(_other); return (*this); }

        auto size() const -> std::size_t;
        void set_max_read(std::size_t _max_read);
        auto chain_size() const -> std::size_t;
        void clear_all();
        void skip(std::size_t _size);

        auto begin() -> iterator;
        auto end() -> iterator;
        auto find(const char* _begin, std::size_t _size) -> iterator;

        // read-write
        void append(buffer& _another);

        auto add(const void* _bytes, std::size_t _size) -> bool;
        auto remove(void* _buffer, std::size_t _length) -> std::size_t;
        
        auto read(util_socket_t _fd, std::int64_t _howmuch) -> std::int64_t;
        auto write(util_socket_t _fd, std::int64_t _howmuch = -1) -> std::int64_t;

        auto add_block(void* _bytes, std::size_t _size) -> bool;
        auto get_block(void** _bytes, std::size_t& _size) -> bool;

    private:
        void move(buffer& _other) noexcept;
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_BUFFER_H_