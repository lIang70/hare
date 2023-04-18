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

#define HARE_MAX_READ_DEFAULT 4096

namespace hare {
namespace io {

    class fixed_block;
    class HARE_API buffer : non_copyable {
        using block = ptr<fixed_block>;
        using block_list = std::list<block>;

        /** @code
         *  +-------++-------++-------++-------++-------++-------+
         *  | block || block || block || block || block || block |
         *  +-------++-------++-------++-------++-------++-------+
         *  |                          |
         *  read_iter                  write_iter
         *  @endcode
         */
        block_list block_chain_ {};
        block_list::iterator read_iter_ { block_chain_.begin() };
        block_list::iterator write_iter_ { block_chain_.begin() };

        size_t total_len_ { 0 };
        size_t max_read_ { HARE_MAX_READ_DEFAULT };

    public:
        using ptr = ptr<buffer>;

        explicit buffer(size_t _max_read = HARE_MAX_READ_DEFAULT);
        ~buffer();

        inline auto size() const -> size_t { return total_len_; }
        inline void set_max_read(size_t _max_read) { max_read_ = _max_read; }

        auto chain_size() const -> size_t;
        void clear_all();

        void append(buffer& _another);
        auto add(const void* _bytes, size_t _size) -> bool;
        auto read(util_socket_t _fd, int64_t _howmuch) -> int64_t;

        auto write(util_socket_t _fd, int64_t _howmuch = -1) -> int64_t;
        auto remove(void* _buffer, size_t _length) -> size_t;

    private:
        auto expand_fast(size_t _howmuch) -> int32_t;
        auto get_block() -> block;
    };

} // namespace io
} // namespace hare

#endif // !_HARE_BASE_IO_BUFFER_H_