#include <cstddef>
#include <cstdint>
#include <hare/base/io/buffer.h>

#include <hare/base/logging.h>
#include <hare/base/util/buffer.h>
#include <hare/hare-config.h>

#include <limits>
#include <memory>
#include <vector>

#ifdef HARE__HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if defined(HARE__HAVE_SYS_UIO_H) || defined(H_OS_WIN32)
#define USE_IOVEC_IMPL
#else
#error "cannot use iovec in this system."
#endif

#ifdef USE_IOVEC_IMPL

#ifdef HARE__HAVE_SYS_UIO_H
#include <sys/uio.h>
#define DEFAULT_WRITE_IOVEC 128
#if defined(UIO_MAXIOV) && UIO_MAXIOV < DEFAULT_WRITE_IOVEC
#define NUM_WRITE_IOVEC UIO_MAXIOV
#elif defined(IOV_MAX) && IOV_MAX < DEFAULT_WRITE_IOVEC
#define NUM_WRITE_IOVEC IOV_MAX
#else
#define NUM_WRITE_IOVEC DEFAULT_WRITE_IOVEC
#endif
#define IOV_TYPE struct iovec
#define IOV_PTR_FIELD iov_base
#define IOV_LEN_FIELD iov_len
#define IOV_LEN_TYPE size_t
#else
#define NUM_WRITE_IOVEC 16
#define IOV_TYPE WSABUF
#define IOV_PTR_FIELD buf
#define IOV_LEN_FIELD len
#define IOV_LEN_TYPE unsigned long
#endif
#endif

#define MAX_TO_COPY_IN_EXPAND 4096
#define MAX_TO_REALIGN_IN_EXPAND 2048
#define MAX_SIZE (std::numeric_limits<std::size_t>::max())
#define MAX_CHINA_SIZE (16)

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

namespace hare {
namespace io {

    namespace detail {
        auto bytes_readable_on_socket(util_socket_t _fd) -> size_t
        {
#if defined(FIONREAD) && defined(H_OS_WIN32)
            // auto reable_cnt = HARE_MAX_READ_DEFAULT;
            // if (::ioctlsocket(target_fd, FIONREAD, &reable_cnt) < 0) {
            //     return (-1);
            // }
            // /* Can overflow, but mostly harmlessly. XXXX */
            // return reable_cnt;
#elif defined(FIONREAD)
            auto reable_cnt = HARE_MAX_READ_DEFAULT;
            if (::ioctl(_fd, FIONREAD, &reable_cnt) < 0) {
                return (-1);
            }
            return reable_cnt;
#else
            return HARE_MAX_READ_DEFAULT;
#endif
        }
    } // namespace detail

    using fixed_buffer = util::fixed_buffer<HARE_LARGE_BUFFER>;

    /**
     * @brief The block of buffer.
     * @code
     * +----------------+-------------+------------------+
     * | misalign bytes |  off bytes  |  residual bytes  |
     * |                |  (CONTENT)  |                  |
     * +----------------+-------------+------------------+
     * |                |             |                  |
     * @endcode
     */
    class fixed_block : public fixed_buffer {
        size_t misalign_ { 0 };

    public:
        inline auto readable() const -> char* { return const_cast<char*>(fixed_buffer::begin()) + misalign_; }
        inline auto readable_size() const -> size_t { return fixed_buffer::length() - misalign_; }
        inline auto full() const -> bool { return readable_size() + misalign_ == static_cast<size_t>(HARE_LARGE_BUFFER); }
        inline auto empty() const -> bool { return readable_size() == 0; }
        inline void clear()
        {
            reset();
            misalign_ = 0;
        }

        void drain(size_t _size)
        {
            HARE_ASSERT(_size <= readable_size(), "oversize.");
            misalign_ += _size;
        }

        auto realign(size_t _size) -> bool
        {
            auto off = readable_size();
            if (fixed_buffer::avail() + misalign_ > _size && off < static_cast<size_t>(HARE_LARGE_BUFFER) / 2 && off <= MAX_TO_REALIGN_IN_EXPAND) {
                ::memmove(fixed_buffer::begin(), readable(), off);
                misalign_ = 0;
                return true;
            }
            return false;
        }

        friend class io::buffer;
    };

    buffer::buffer(std::size_t max_read)
        : max_read_(max_read)
    {
    }

    buffer::~buffer() = default;

    auto buffer::chain_size() const -> size_t
    {
        return block_chain_.size();
    }

    void buffer::clear_all()
    {
        /// FIXME: clear directly?
        block_chain_.clear();
        read_iter_ = block_chain_.begin();
        write_iter_ = block_chain_.begin();
        total_len_ = 0;
    }

    void buffer::append(buffer& _another)
    {
        if (_another.total_len_ == 0) {
            return;
        }

        total_len_ += _another.total_len_;
        _another.total_len_ = 0;
        auto empty = block_chain_.empty();

        if (!empty && !(*write_iter_)->empty()) {
            ++write_iter_;
            if (write_iter_ == block_chain_.end()) {
                write_iter_ = block_chain_.begin();
            }
        }

        if (read_iter_ == write_iter_) {
            ++write_iter_;
            if (write_iter_ == block_chain_.end()) {
                write_iter_ = block_chain_.begin();
            }
            block_chain_.insert(write_iter_, std::make_shared<fixed_block>());
            --write_iter_;
        }

        while (_another.read_iter_ != _another.write_iter_) {
            auto block = std::move(*_another.read_iter_);
            _another.block_chain_.erase(_another.read_iter_++);
            if (_another.read_iter_ == _another.block_chain_.end()) {
                _another.read_iter_ = _another.block_chain_.begin();
            }
            if (!block->empty()) {
                block_chain_.insert(write_iter_, block);
            } else {
                break;
            }
        }

        if (!(*_another.read_iter_)->empty()) {
            HARE_ASSERT(_another.read_iter_ == _another.write_iter_, "");
            auto block = std::move(*_another.read_iter_);
            _another.clear_all();
            block_chain_.insert(write_iter_, block);
        }

        if (!empty && (*write_iter_)->empty()) {
            --write_iter_;
        } else if (empty) {
            read_iter_ = block_chain_.begin();
            write_iter_ = block_chain_.end();
            --write_iter_;
        }
    }

    auto buffer::add(const void* _bytes, size_t _size) -> bool
    {
        if (total_len_ + _size > MAX_SIZE) {
            return false;
        }

        total_len_ += _size;
        auto curr_block = get_block();
        auto remain = curr_block->avail();
        const auto* bytes = static_cast<const char*>(_bytes);
        do {
            auto real_size = MIN(remain, _size);
            curr_block->append(bytes, real_size);
            _size = _size < real_size ? 0 : _size - real_size;
            bytes += real_size;
            curr_block = get_block();
            remain = curr_block->avail();
        } while (_size > 0);

        return true;
    }

    auto buffer::read(util_socket_t _fd, int64_t _howmuch) -> int64_t
    {
        auto readable = detail::bytes_readable_on_socket(_fd);
        if (readable <= 0 || readable > max_read_) {
            readable = max_read_;
        }
        if (_howmuch < 0 || _howmuch > readable) {
            _howmuch = static_cast<int64_t>(readable);
        }

#ifdef USE_IOVEC_IMPL
        auto nvecs = expand_fast(_howmuch);

        IOV_TYPE vecs[nvecs];

        auto iter = write_iter_;
        auto index = 0;
        for (; index < nvecs && iter != block_chain_.end(); ++index) {
            auto block = (*iter);
            if (!block->full()) {
                vecs[index].IOV_PTR_FIELD = block->current();
                vecs[index].IOV_LEN_FIELD = block->avail();
            }
            ++iter;
            if (iter == block_chain_.end()) {
                iter = block_chain_.begin();
            }
        }

        readable = ::readv(_fd, vecs, index);

#endif

        if (readable <= 0) {
            return static_cast<int64_t>(readable);
        }

#ifdef USE_IOVEC_IMPL
        auto remaining = readable;
        iter = write_iter_;
        for (auto i = 0; i < nvecs; ++i) {
            auto space = (*iter)->avail();
            if (space < remaining) {
                (*iter)->add(space);
                remaining -= space;
            } else {
                (*iter)->add(remaining);
                write_iter_ = iter;
                break;
            }
            ++iter;
            if (iter == block_chain_.end()) {
                iter = block_chain_.begin();
            }
        }
#endif
        total_len_ += readable;
        return static_cast<int64_t>(readable);
    }

    auto buffer::write(util_socket_t _fd, int64_t _howmuch) -> int64_t
    {
        auto write_n = static_cast<ssize_t>(0);

        if (_howmuch < 0 || _howmuch > total_len_) {
            _howmuch = static_cast<int64_t>(total_len_);
        }

        if (_howmuch > 0) {

#ifdef USE_IOVEC_IMPL
            IOV_TYPE iov[NUM_WRITE_IOVEC];
            int32_t write_i = 0;
            auto curr = read_iter_;

            while (write_i < NUM_WRITE_IOVEC && (_howmuch != 0)) {
                iov[write_i].IOV_PTR_FIELD = (*curr)->readable();
                if (_howmuch >= (*curr)->readable_size()) {
                    /* XXXcould be problematic when windows supports mmap*/
                    iov[write_i++].IOV_LEN_FIELD = static_cast<IOV_LEN_TYPE>((*curr)->readable_size());
                    _howmuch -= static_cast<int64_t>((*curr)->readable_size());
                } else {
                    /* XXXcould be problematic when windows supports mmap*/
                    iov[write_i++].IOV_LEN_FIELD = static_cast<IOV_LEN_TYPE>(_howmuch);
                    break;
                }
                ++curr;
                if (curr == block_chain_.end()) {
                    curr = block_chain_.begin();
                }
            }
            if (write_i == 0) {
                write_n = 0;
                return (0);
            }

            write_n = ::writev(_fd, iov, write_i);
            total_len_ -= write_n;

            /**
             * @brief If the length of chain bigger than MAX_CHINA_SIZE,
             *  cleaning will begin.
             **/
            auto clean = chain_size() > MAX_CHINA_SIZE;
            auto length = static_cast<size_t>(write_n);
            auto curr_block = *read_iter_;
            while (length != 0U && length > curr_block->readable_size()) {
                length = length < curr_block->readable_size() ? 0 : length - curr_block->readable_size();
                if (clean) {
                    HARE_ASSERT(read_iter_ != write_iter_, "buffer over size.");
                    block_chain_.erase(read_iter_++);
                } else {
                    curr_block->clear();
                    ++read_iter_;
                }
                if (read_iter_ == block_chain_.end()) {
                    read_iter_ = block_chain_.begin();
                }
                curr_block = *read_iter_;
            }

            if (length != 0U) {
                HARE_ASSERT(length <= curr_block->readable_size(), "buffer over size.");
                curr_block->drain(length);
            }

#endif
        }
        return (write_n);
    }

    auto buffer::remove(void* _buffer, size_t _length) -> size_t
    {
        if (_length == 0) {
            return _length;
        }

        if (_length > total_len_) {
            _length = total_len_;
        }

        /**
         * @brief If the length of chain bigger than MAX_CHINA_SIZE,
         *  cleaning will begin.
         **/
        auto clean = chain_size() > MAX_CHINA_SIZE;
        auto* buffer = static_cast<char*>(_buffer);
        auto curr_block = *read_iter_;
        auto nread = _length;

        while (_length != 0U && _length > curr_block->readable_size()) {
            auto copylen = curr_block->readable_size();
            memcpy(buffer, curr_block->readable(), copylen);
            if (!clean) {
                curr_block->clear();
            }
            buffer += copylen;
            _length -= copylen;

            if (clean) {
                HARE_ASSERT(read_iter_ != write_iter_, "buffer over size.");
                block_chain_.erase(read_iter_++);
            } else {
                ++read_iter_;
            }
            if (read_iter_ == block_chain_.end()) {
                read_iter_ = block_chain_.begin();
            }
            curr_block = *read_iter_;
        }

        if (_length != 0U) {
            HARE_ASSERT(_length <= curr_block->readable_size(), "buffer over size.");
            memcpy(buffer, curr_block->readable(), _length);
            curr_block->drain(_length);
        }

        return nread;
    }

    auto buffer::expand_fast(size_t _howmuch) -> int32_t
    {
        auto ret { 0 };
        auto res = _howmuch;
        auto cur_iter = write_iter_;

        if (block_chain_.empty()) {
            HARE_ASSERT(write_iter_ == block_chain_.end(), "");
            HARE_ASSERT(read_iter_ == block_chain_.end(), "");
            ret = static_cast<int32_t>(res / static_cast<size_t>(HARE_LARGE_BUFFER));
            block_chain_.push_back(std::make_shared<fixed_block>());
            block_chain_.insert(block_chain_.begin(), ret, std::make_shared<fixed_block>());
            write_iter_ = block_chain_.begin();
            read_iter_ = block_chain_.begin();
            return ret + 1;
        }

        if ((*cur_iter)->avail() > 0) {
            ++ret;
            res = (*cur_iter)->avail() > res ? 0 : res - (*cur_iter)->avail();
            ++cur_iter;
            if (cur_iter == block_chain_.end()) {
                cur_iter = block_chain_.begin();
            }
        }

        if (res == 0) {
            return ret;
        }

        ret += static_cast<int32_t>(res / static_cast<size_t>(HARE_LARGE_BUFFER)) + 1;
        auto index = ret - 1;
        while (index != 0 && cur_iter != read_iter_) {
            HARE_ASSERT((*cur_iter)->empty(), "the block must be empty.");
            --index;
            ++cur_iter;
            if (cur_iter == block_chain_.end()) {
                cur_iter = block_chain_.begin();
            }
        }
        block_chain_.insert(block_chain_.begin(), index, std::make_shared<fixed_block>());

        return ret;
    }

    auto buffer::get_block() -> block
    {
        if (block_chain_.empty()) {
            HARE_ASSERT(write_iter_ == block_chain_.end(), "");
            HARE_ASSERT(read_iter_ == block_chain_.end(), "");
            block_chain_.emplace_back(new fixed_block);
            write_iter_ = block_chain_.begin();
            read_iter_ = block_chain_.begin();
            return (*write_iter_);
        }

        for (; write_iter_ != read_iter_;) {
            auto cur_block = (*write_iter_);
            if (!cur_block->full()) {
                return cur_block;
            }
            ++write_iter_;
            if (write_iter_ == block_chain_.end()) {
                write_iter_ = block_chain_.begin();
            }
        }

        if (chain_size() == 1) {
            block_chain_.insert(write_iter_, std::make_shared<fixed_block>());
            --write_iter_;
        } else {
            ++write_iter_;
            if (write_iter_ == block_chain_.end()) {
                write_iter_ = block_chain_.begin();
            }
        }
        HARE_ASSERT((*write_iter_)->empty(), "the block is not empty.");
        return (*write_iter_);
    }

} // namespace io
} // namespace hare