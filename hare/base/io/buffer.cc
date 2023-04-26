#include <hare/base/io/buffer.h>

#include <hare/base/logging.h>
#include <hare/hare-config.h>

#include <limits>
#include <vector>

#if HARE__HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if defined(HARE__HAVE_SYS_UIO_H) || defined(H_OS_WIN32)
#define USE_IOVEC_IMPL
#else
#error "cannot use iovec in this system."
#endif

#ifdef USE_IOVEC_IMPL

#if HARE__HAVE_SYS_UIO_H
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

#define MIN_TO_ALLOC 512
#define MAX_TO_COPY_IN_EXPAND 4096
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

        static auto bytes_readable_on_socket(util_socket_t _fd) -> int64_t
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

        static auto round_up(size_t _size) -> size_t
        {
            size_t size { MIN_TO_ALLOC };
            if (_size < MAX_SIZE / 2) {
                while (size < _size) {
                    size <<= 1;
                }
            } else {
                size = _size;
            }
            return size;
        }

        class buffer_block : public cache {
            char* data_ { nullptr };
            char* cur_ { nullptr };
            size_t max_size_ { 0 };

        public:
            explicit buffer_block(size_t _max_size)
                : max_size_(_max_size)
                , data_(new char[_max_size])
            {
                cur_ = data_;
            }

            ~buffer_block() override { delete[] data_; }

            void append(const char* _buf, size_t _length) override
            {
                if (implicit_cast<size_t>(avail()) > _length) {
                    memcpy(cur_, _buf, _length);
                    cur_ += _length;
                }
            }

            auto begin() -> char* override { return data_; }
            auto begin() const -> const char* override { return data_; }
            auto size() const -> size_t override { return static_cast<size_t>(cur_ - data_); }
            auto max_size() const -> size_t override { return max_size_; }

            // write to data_ directly
            auto current() -> char* override { return cur_; }
            auto current() const -> char* override { return cur_; }
            auto avail() const -> size_t override { return static_cast<size_t>(end() - cur_); }
            void skip(size_t _length) override { cur_ += _length; }

            void reset() override { cur_ = data_; }
            void bzero() override { set_zero(data_, max_size_); }

        private:
            auto end() const -> const char* { return data_ + max_size_; }
        };

    } // namespace detail

    buffer::buffer(int64_t _max_read)
        : max_read_(_max_read)
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
        write_iter_ = block_chain_.begin();
        total_len_ = 0;
    }

    void buffer::append(buffer& _another)
    {
        if (_another.total_len_ == 0) {
            return;
        }

        total_len_ += _another.total_len_;

        if (block_chain_.empty() || (write_iter_ == block_chain_.begin() && (*write_iter_)->empty())) {
            block_chain_.swap(_another.block_chain_);
            _another.write_iter_ = _another.block_chain_.begin();
            _another.total_len_ = 0;
            write_iter_ = block_chain_.end();
            --write_iter_;
            return;
        }

        if (!(*write_iter_)->empty()) {
            ++write_iter_;
        }

        ++_another.write_iter_;
        auto cur = _another.block_chain_.begin();
        while (cur != _another.write_iter_) {
            block_chain_.insert(write_iter_, std::move(*cur));
            ++cur;
        }

        --write_iter_;
        _another.block_chain_.clear();
        _another.write_iter_ = _another.block_chain_.begin();
        _another.total_len_ = 0;
    }

    auto buffer::add(const void* _bytes, size_t _size) -> bool
    {
        if (total_len_ + _size > MAX_SIZE) {
            return false;
        }

        total_len_ += _size;

        auto cur { write_iter_ };
        if (block_chain_.empty()) {
            HARE_ASSERT(write_iter_ == block_chain_.end(), "");
            block_chain_.emplace_back(new detail::buffer_block(detail::round_up(_size)));
            write_iter_ = block_chain_.begin();
            cur = write_iter_;
        }

        if ((*cur)->avail() < _size && !(*cur)->realign(_size)) {
            ++cur;
            while (cur != block_chain_.end()) {
                HARE_ASSERT((*cur)->empty(), "error in buffer.");
                if ((*cur)->max_size() > _size) {
                    break;
                }
                block_chain_.erase(cur++);
            }
            if (cur == block_chain_.end()) {
                block_chain_.insert(cur, std::make_shared<detail::buffer_block>(detail::round_up(_size)));
            }
            --cur;
        }

        /**
         * @brief there's enough space to hold all the data
         *   in thecurrent last chain.
         **/
        (*cur)->append(static_cast<const char*>(_bytes), _size);
        write_iter_ = cur;

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
        std::vector<IOV_TYPE> vecs(2);

        auto iter = write_iter_;
        auto vec_nbr = 0;
        auto remain = readable;
        for (; remain > 0; ++vec_nbr) {
            if (vec_nbr >= 2) {
                vecs.emplace_back();
            }
            if (iter == block_chain_.end()) {
                block_chain_.insert(iter, std::make_shared<detail::buffer_block>(detail::round_up(remain)));
                --iter;
                if (write_iter_ == block_chain_.end()) {
                    write_iter_ = block_chain_.begin();
                }
            }
            auto block = (*iter);
            if (!block->full()) {
                remain -= static_cast<int64_t>(block->avail());
                vecs[vec_nbr].IOV_PTR_FIELD = block->current();
                vecs[vec_nbr].IOV_LEN_FIELD = block->avail();
            }
            ++iter;
        }

        readable = ::readv(_fd, vecs.data(), vec_nbr);

        if (readable > 0) {
            remain = readable;
            iter = write_iter_;
            for (auto i = 0; i < vec_nbr; ++i) {
                auto space = (*iter)->avail();
                if (space < remain) {
                    (*iter)->skip(space);
                    remain -= static_cast<int64_t>(space);
                } else {
                    (*iter)->skip(remain);
                    write_iter_ = iter;
                    break;
                }
                ++iter;
            }
        }

#endif

        if (readable <= 0) {
            return readable;
        }
        total_len_ += static_cast<size_t>(readable);
        return readable;
    }

    auto buffer::write(util_socket_t _fd, int64_t _howmuch) -> int64_t
    {
        auto write_n = static_cast<ssize_t>(0);

        if (_howmuch < 0 || _howmuch > total_len_) {
            _howmuch = static_cast<int64_t>(total_len_);
        }

        if (_howmuch > 0) {
            HARE_ASSERT(!block_chain_.empty(), "buffer is empty.");
#ifdef USE_IOVEC_IMPL
            std::array<IOV_TYPE, NUM_WRITE_IOVEC> iov {};
            auto curr { block_chain_.begin() };
            auto write_i { 0 };

            while (write_i < NUM_WRITE_IOVEC && _howmuch > 0) {
                HARE_ASSERT(curr != block_chain_.end(), "iterator overflow.");
                iov[write_i].IOV_PTR_FIELD = (*curr)->readable();
                if (_howmuch > (*curr)->readable_size()) {
                    /* XXXcould be problematic when windows supports mmap*/
                    iov[write_i++].IOV_LEN_FIELD = static_cast<IOV_LEN_TYPE>((*curr)->readable_size());
                    _howmuch -= static_cast<int64_t>((*curr)->readable_size());
                } else {
                    /* XXXcould be problematic when windows supports mmap*/
                    iov[write_i++].IOV_LEN_FIELD = static_cast<IOV_LEN_TYPE>(_howmuch);
                    _howmuch -= static_cast<int64_t>((*curr)->readable_size());
                    break;
                }
                ++curr;
            }

            if (write_i == 0) {
                write_n = 0;
                return (0);
            }

            write_n = ::writev(_fd, iov.data(), write_i);
            total_len_ -= write_n;

            /**
             * @brief If the length of chain bigger than MAX_CHINA_SIZE,
             *   cleaning will begin.
             **/
            auto clean = chain_size() > MAX_CHINA_SIZE;
            auto remain = static_cast<size_t>(write_n);
            curr = block_chain_.begin();
            auto curr_block = *curr;
            while (remain != 0U && remain > curr_block->readable_size()) {
                HARE_ASSERT(curr != block_chain_.end(), "iterator overflow.");
                remain = remain < curr_block->readable_size() ? 0 : remain - curr_block->readable_size();
                if (!clean) {
                    curr_block->clear();
                    block_chain_.emplace_back(std::move(*curr));
                }
                block_chain_.erase(curr++);
                curr_block = *curr;
            }

            if (remain != 0U) {
                HARE_ASSERT(remain <= curr_block->readable_size(), "buffer over size.");
                curr_block->drain(remain);
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
        auto curr = block_chain_.begin();
        auto curr_block = *curr;
        auto* buffer = static_cast<char*>(_buffer);
        size_t nread { 0 };

        while (_length != 0U && _length > curr_block->readable_size()) {
            HARE_ASSERT(curr != block_chain_.end(), "iterator overflow.");
            auto copy_len = curr_block->readable_size();
            ::memcpy(buffer, curr_block->readable(), copy_len);
            buffer += copy_len;
            _length -= copy_len;
            nread += copy_len;

            if (!clean) {
                curr_block->clear();
                block_chain_.emplace_back(std::move(*curr));
            }
            block_chain_.erase(curr++);
            curr_block = *curr;
        }

        if (_length != 0U) {
            HARE_ASSERT(_length <= curr_block->readable_size(), "buffer over size.");
            ::memcpy(buffer, curr_block->readable(), _length);
            curr_block->drain(_length);
            nread += _length;
        }

        return nread;
    }

} // namespace io
} // namespace hare