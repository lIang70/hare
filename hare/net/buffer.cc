#include "hare/net/socket_op.h"
#include <hare/base/exception.h>
#include <hare/base/util/buffer.h>
#include <hare/hare-config.h>
#include <hare/net/buffer.h>

#include <cassert>
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
#define IOV_LEN_TYPE std::size_t
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
#define MAX_TO_REALIGN 2048
#define MAX_SIZE (std::numeric_limits<std::size_t>::max())
#define MAX_CHINA_SIZE (16)

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

namespace hare {
namespace net {

    namespace detail {
        static auto round_up(std::size_t _size) -> std::size_t
        {
            std::size_t size { MIN_TO_ALLOC };
            if (_size < MAX_SIZE / 2) {
                while (size < _size) {
                    size <<= 1;
                }
            } else {
                size = _size;
            }
            return size;
        }

        static auto get_next(const char* _begin, std::size_t _size) -> std::vector<int>
        {
            std::vector<int> next(_size, 0);
            int k = -1;
            for (int j = 0; j < _size;) {
                if (k == -1 || _begin[j] == _begin[k]) {
                    ++j, ++k;
                    if (_begin[j] != _begin[k]) {
                        next[j] = k;
                    } else {
                        next[j] = next[k];
                    }
                } else {
                    k = next[k];
                }
            }
            return next;
        }

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
        class cache : public util::buffer<char> {
            std::size_t misalign_ { 0 };

        private:
            void grow(std::size_t capacity) override { }

        public:
            using base = util::buffer<char>;

            explicit cache(std::size_t _max_size)
                : base(new base::value_type[_max_size], 0, _max_size)
            {
            }

            cache(base::value_type* _data, std::size_t _max_size)
                : base(_data, _max_size, _max_size)
            {
            }

            virtual ~cache() { delete[] begin(); }

            // write to data_ directly
            inline auto writeable() -> base::value_type* { return begin() + size(); }
            inline auto writeable() const -> const base::value_type* { return begin() + size(); }
            inline auto writeable_size() const -> std::size_t { return capacity() - size(); }

            inline auto readable() -> base::value_type* { return data() + misalign_; }
            inline auto readable_size() const -> std::size_t { return size() - misalign_; }
            inline auto full() const -> bool { return readable_size() + misalign_ == capacity(); }
            inline auto empty() const -> bool { return readable_size() == 0; }
            inline void clear()
            {
                base::clear();
                misalign_ = 0;
            }

            inline void drain(std::size_t _size)
            {
                misalign_ += _size;
            }

            auto realign(std::size_t _size) -> bool
            {
                auto offset = readable_size();
                if (writeable_size() + misalign_ > _size && offset <= MAX_TO_REALIGN) {
                    ::memmove(begin(), readable(), offset);
                    set(begin(), offset);
                    misalign_ = 0;
                    return true;
                }
                return false;
            }

            inline void bzero() { hare::detail::fill_n(data(), capacity(), 0); }

        private:
            auto end() const -> const base::value_type* { return data() + capacity(); }

            friend class net::buffer;
        };

    } // namespace detail

    buffer::buffer(std::size_t _max_read)
        : max_read_(_max_read)
    {
    }

    buffer::~buffer() = default;

    auto buffer::operator[](std::size_t _index) -> char
    {
        if (_index > total_len_) {
            throw exception("over buffer size.");
        }

        auto iter = block_chain_.begin();

        while (_index > (*iter)->size()) {
            _index -= (*iter)->size();
            ++iter;
        }

        assert(iter != block_chain_.end());
        assert(_index <= (*iter)->size());

        return (*iter)->readable()[_index];
    }

    auto buffer::chain_size() const -> std::size_t
    {
        return block_chain_.size();
    }

    void buffer::clear_all()
    {
        block_chain_.clear();
        write_iter_ = block_chain_.begin();
        total_len_ = 0;
    }

    auto buffer::find(const char* _begin, std::size_t _size) -> int64_t
    {
        auto next_val = detail::get_next(_begin, _size);
        auto i = 0, j = 0;
        while (i < total_len_ && j < _size) {
            if (j == -1 || (*this)[i] == _begin[j]) {
                ++i, ++j;
            } else {
                j = next_val[j];
            }
        }

        if (j >= _size) {
            return static_cast<int64_t>(static_cast<std::size_t>(i) - _size);
        }
        return -1;
    }

    void buffer::skip(std::size_t _size)
    {
        if (_size >= total_len_) {
            total_len_ = 0;
            block_chain_.clear();
            goto done;
        }

        while (block_chain_.front()->readable_size() <= _size) {
            assert(!block_chain_.empty());
            _size -= block_chain_.front()->readable_size();
            block_chain_.pop_front();
        }

        assert(_size < 0);

        if (_size > 0) {
            assert(!block_chain_.empty());
            assert(block_chain_.front()->readable_size() > _size);
            block_chain_.front()->drain(_size);
        }

    done:
        write_iter_ = block_chain_.begin();
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

    auto buffer::add(const void* _bytes, std::size_t _size) -> bool
    {
        if (total_len_ + _size > MAX_SIZE) {
            return false;
        }

        total_len_ += _size;

        auto cur { write_iter_ };
        if (block_chain_.empty()) {
            assert(write_iter_ == block_chain_.end());
            block_chain_.emplace_back(new detail::cache(detail::round_up(_size)));
            write_iter_ = block_chain_.begin();
            cur = write_iter_;
        }

        if ((*cur)->writeable_size() < _size && !(*cur)->realign(_size)) {
            ++cur;
            while (cur != block_chain_.end()) {
                assert((*cur)->empty());
                if ((*cur)->capacity() > _size) {
                    break;
                }
                block_chain_.erase(cur++);
            }
            if (cur == block_chain_.end()) {
                block_chain_.emplace(cur, new detail::cache(detail::round_up(_size)));
            }
            --cur;
        }

        /**
         * @brief there's enough space to hold all the data
         *   in thecurrent last chain.
         **/
        assert((*cur)->writeable_size() > _size);
        (*cur)->append(static_cast<const char*>(_bytes),
            static_cast<const char*>(_bytes) + _size);
        write_iter_ = cur;

        return true;
    }

    auto buffer::read(util_socket_t _fd, int64_t _howmuch) -> int64_t
    {
        std::int64_t readable = socket_op::get_bytes_readable_on_socket(_fd);
        if (readable <= 0 || readable > max_read_) {
            readable = static_cast<int64_t>(max_read_);
        }
        if (_howmuch < 0 || _howmuch > readable) {
            _howmuch = readable;
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
                block_chain_.emplace(iter, new detail::cache(detail::round_up(remain)));
                --iter;
                if (write_iter_ == block_chain_.end()) {
                    write_iter_ = block_chain_.begin();
                }
            }
            auto& block = (*iter);
            if (!block->full()) {
                remain -= static_cast<int64_t>(block->writeable_size());
                vecs[vec_nbr].IOV_PTR_FIELD = block->writeable();
                vecs[vec_nbr].IOV_LEN_FIELD = block->writeable_size();
            }
            ++iter;
        }

        readable = ::readv(_fd, vecs.data(), vec_nbr);

        if (readable > 0) {
            remain = readable;
            iter = write_iter_;
            for (auto i = 0; i < vec_nbr; ++i) {
                auto space = (*iter)->writeable_size();
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
#else
#error "cannot use IOVEC."
#endif

        if (readable <= 0) {
            return readable;
        }
        total_len_ += static_cast<std::size_t>(readable);
        return readable;
    }

    auto buffer::write(util_socket_t _fd, int64_t _howmuch) -> int64_t
    {
        std::size_t write_n {};

        if (_howmuch < 0 || _howmuch > total_len_) {
            _howmuch = static_cast<int64_t>(total_len_);
        }

        if (_howmuch > 0) {
            assert(!block_chain_.empty());
#ifdef USE_IOVEC_IMPL
            std::array<IOV_TYPE, NUM_WRITE_IOVEC> iov {};
            auto curr { block_chain_.begin() };
            auto write_i { 0 };

            while (write_i < NUM_WRITE_IOVEC && _howmuch > 0) {
                assert(curr != block_chain_.end());
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
            auto remain = write_n;
            curr = block_chain_.begin();
            while (remain != 0U && remain > (*curr)->readable_size()) {
                assert(curr != block_chain_.end());
                auto& curr_block = *curr;
                remain = remain < curr_block->readable_size() ? 0 : remain - curr_block->readable_size();
                if (!clean) {
                    curr_block->clear();
                    block_chain_.emplace_back(std::move(*curr));
                }
                block_chain_.erase(curr++);
            }

            if (remain != 0U) {
                assert(remain <= (*curr)->readable_size());
                (*curr)->drain(remain);
            }
#endif
        }
        return static_cast<int64_t>(write_n);
    }

    auto buffer::remove(void* _buffer, std::size_t _length) -> std::size_t
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
        auto* buffer = static_cast<uint8_t*>(_buffer);
        size_t nread { 0 };

        while (_length != 0U && _length > (*curr)->readable_size()) {
            assert(curr != block_chain_.end());
            auto& curr_block = *curr;
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
        }

        if (_length != 0U) {
            assert(_length <= (*curr)->readable_size());
            ::memcpy(buffer, (*curr)->readable(), _length);
            (*curr)->drain(_length);
            nread += _length;
        }

        total_len_ -= nread;
        return nread;
    }

    auto buffer::add_block(void* _bytes, size_t _size) -> bool
    {
        block_chain_.emplace_back(new detail::cache(static_cast<char*>(_bytes), _size));
        total_len_ += _size;
        return true;
    }

    auto buffer::get_block(void** _bytes, size_t& _size) -> bool
    {
        if (block_chain_.empty()) {
            *_bytes = nullptr;
            _size = 0;
            return false;
        }

        auto bolck = std::move(block_chain_.front());
        block_chain_.pop_front();

        *_bytes = bolck->begin();
        _size = bolck->capacity();
        bolck->set(nullptr, 0);

        total_len_ -= _size;
        return true;
    }

} // namespace net
} // namespace hare