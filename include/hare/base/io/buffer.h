/**
 * @file hare/base/io/buffer.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with buffer.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_IO_BUFFER_H_
#define _HARE_BASE_IO_BUFFER_H_

#include <hare/base/util/non_copyable.h>

#include <array>
#include <list>
#include <string>

#define HARE_MAX_READ_DEFAULT 4096
#define HARE_MAX_TO_REALIGN 2048

namespace hare {
namespace io {

    /**
     * @brief The block of buffer.
     * @code
     * +----------------+-------------+------------------+
     * | misalign bytes |  off bytes  |  residual bytes  |
     * |                |  (CONTENT)  |                  |
     * +----------------+-------------+------------------+
     * |                |             |                  |
     * @endcode
     **/
    class HARE_API cache {
        size_t misalign_ { 0 };

    public:
        virtual ~cache() = default;

        virtual void append(const char* _buf, size_t _length) = 0;
        virtual auto begin() -> char* = 0;
        virtual auto begin() const -> const char* = 0;
        virtual auto size() const -> size_t = 0;
        virtual auto max_size() const -> size_t = 0;

        // write to data_ directly
        virtual auto current() -> char* = 0;
        virtual auto current() const -> char* = 0;
        virtual auto avail() const -> size_t = 0;
        virtual void skip(size_t _length) = 0;

        inline auto readable() const -> char* { return const_cast<char*>(begin()) + misalign_; }
        inline auto readable_size() const -> size_t { return size() - misalign_; }
        inline auto full() const -> bool { return readable_size() + misalign_ == max_size(); }
        inline auto empty() const -> bool { return readable_size() == 0; }
        inline void clear()
        {
            reset();
            misalign_ = 0;
        }

        void drain(size_t _size)
        {
            misalign_ += _size;
        }

        auto realign(size_t _size) -> bool
        {
            auto off = readable_size();
            if (avail() + misalign_ > _size && 
                off < static_cast<size_t>(HARE_LARGE_BUFFER) / 2 && 
                off <= HARE_MAX_TO_REALIGN) {
                ::memmove(begin(), readable(), off);
                misalign_ = 0;
                return true;
            }
            return false;
        }

        virtual void reset() = 0;
        virtual void bzero() = 0;
    };

    template <size_t SIZE>
    class HARE_API fixed_cache : public non_copyable, public cache {
        void (*cookie_)() { nullptr };
        std::array<char, SIZE> data_ {};
        char* cur_ { nullptr };

    public:
        using ptr = ptr<fixed_cache<SIZE>>;

        fixed_cache()
            : cur_(data_.data())
        {
            set_cookie(cookie_start);
        }

        ~fixed_cache() override
        {
            set_cookie(cookie_end);
        }

        void append(const char* _buf, size_t _length) override
        {
            if (implicit_cast<size_t>(avail()) > _length) {
                memcpy(cur_, _buf, _length);
                cur_ += _length;
            }
        }

        auto begin() -> char* override { return data_.data(); }
        auto begin() const -> const char* override { return data_.data(); }
        auto size() const -> size_t override { return static_cast<size_t>(cur_ - data_.data()); }
        auto max_size() const -> size_t override { return SIZE; }

        // write to data_ directly
        auto current() -> char* override { return cur_; }
        auto current() const -> char* override { return cur_; }
        auto avail() const -> size_t override { return static_cast<size_t>(end() - cur_); }
        void skip(size_t _length) override { cur_ += _length; }

        void reset() override { cur_ = data_.data(); }
        void bzero() override { set_zero(data_.data(), SIZE); }

        void set_cookie(void (*_cookie)()) { cookie_ = _cookie; }

        auto to_string() const -> std::string { return { data_.begin(), size() }; }

    private:
        auto end() const -> const char* { return data_.data() + SIZE; }
        static void cookie_start() { }
        static void cookie_end() { }
        
    };

    class HARE_API buffer : public non_copyable {
        using block = ptr<cache>;
        using block_list = std::list<block>;

        /** @code
         *  +-------++-------++-------++-------++-------++-------+
         *  | block || block || block || block || block || block |
         *  +-------++-------++-------++-------++-------++-------+
         *                             |
         *                             write_iter
         *  @endcode
         **/
        block_list block_chain_ {};
        block_list::iterator write_iter_ { block_chain_.begin() };

        size_t total_len_ { 0 };
        int64_t max_read_ { HARE_MAX_READ_DEFAULT };

    public:
        using ptr = ptr<buffer>;

        explicit buffer(int64_t _max_read = HARE_MAX_READ_DEFAULT);
        ~buffer();

        inline auto size() const -> size_t { return total_len_; }
        inline void set_max_read(int64_t _max_read) { max_read_ = _max_read; }

        auto chain_size() const -> size_t;
        void clear_all();

        void append(buffer& _another);

        // for tcp
        auto add(const void* _bytes, size_t _size) -> bool;
        auto read(util_socket_t _fd, int64_t _howmuch) -> int64_t;
        auto write(util_socket_t _fd, int64_t _howmuch = -1) -> int64_t;
        auto remove(void* _buffer, size_t _length) -> size_t;
    };

} // namespace io
} // namespace hare

#endif // !_HARE_BASE_IO_BUFFER_H_