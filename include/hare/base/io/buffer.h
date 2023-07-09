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

#include <list>
#include <string>

#define HARE_MAX_READ_DEFAULT 4096
#define HARE_MAX_TO_REALIGN 2048

namespace hare {
namespace io {

    namespace detail {
        struct cache;
    } // namespace detail

    class HARE_API buffer : public non_copyable {
        using block = uptr<detail::cache>;
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

        auto operator[](size_t _index) -> char;

        auto chain_size() const -> size_t;
        void clear_all();

        auto find(const char* _begin, size_t _size) -> int64_t;
        void skip(size_t _size);

        void append(buffer& _another);

        // for tcp
        auto add(const void* _bytes, size_t _size) -> bool;
        auto read(util_socket_t _fd, int64_t _howmuch) -> int64_t;
        auto write(util_socket_t _fd, int64_t _howmuch = -1) -> int64_t;
        auto remove(void* _buffer, size_t _length) -> size_t;

        // for udp
        auto add_block(void* _bytes, size_t _size) -> bool;
        auto get_block(void** _bytes, size_t& _size) -> bool;
    };

} // namespace io
} // namespace hare

#endif // !_HARE_BASE_IO_BUFFER_H_