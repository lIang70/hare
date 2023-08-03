#include "hare/base/fwd-inl.h"
#include "hare/base/io/socket_op-inl.h"
#include "hare/net/buffer-inl.h"
#include <hare/base/exception.h>
#include <hare/hare-config.h>

#include <cassert>
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
#define MAX_TO_ALLOC (4096UL * 16)
#define IOV_TYPE struct iovec
#define IOV_PTR_FIELD iov_base
#define IOV_LEN_FIELD iov_len
#define IOV_LEN_TYPE std::size_t
#else
#include <WinSock2.h>
#define MAX_TO_ALLOC (4096U * 16)
#define NUM_WRITE_IOVEC 16
#define IOV_TYPE WSABUF
#define IOV_PTR_FIELD buf
#define IOV_LEN_FIELD len
#define IOV_LEN_TYPE unsigned long
#endif
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

        // kmp
        static auto get_next(const char* _begin, std::size_t _size) -> std::vector<std::int32_t>
        {
            std::vector<std::int32_t> next(_size, -1);
            std::int32_t k = -1;
            for (std::size_t j = 0; j < _size;) {
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

        auto cache::realign(std::size_t _size) -> bool
        {
            auto offset = readable_size();
            if (writeable_size() >= _size) {
                return true;
            } else if (writeable_size() + misalign_ > _size && offset <= MAX_TO_REALIGN) {
                ::memmove(begin(), readable(), offset);
                set(begin(), offset);
                misalign_ = 0;
                return true;
            }
            return false;
        }

        void cache_list::check_size(std::size_t _size)
        {
            if (!write->cache) {
                write->cache.reset(new cache(round_up(_size)));
            } else if (!(*write)->realign(_size)) {
                get_next_write();
                if (!write->cache || (*write)->size() < _size) {
                    write->cache.reset(new cache(round_up(_size)));
                }
            }
        }

        void cache_list::append(cache_list& _other)
        {
            auto* other_begin = _other.begin();
            auto* other_end = _other.end();
            auto chain_size = 1;

            while (other_begin != other_end) {
                ++chain_size;
                other_begin = other_begin->next;
            }
            other_begin = _other.begin();

            if (other_begin->prev == other_end) {
                //   ---+---++---+---
                // .... |   ||   | ....
                //   ---+---++---+---
                //        ^    ^
                //      write read
                end()->next = other_begin;
                other_begin->prev = end();
                begin()->prev = other_end;
                other_end->next = begin();
                write = other_end;

                // reset other
                _other.node_size_ = 1;
                _other.head = new detail::cache_list::node;
                _other.head->next = _other.head;
                _other.head->prev = _other.head;
                _other.read = _other.head;
                _other.write = _other.head;
            } else {
                //   ---+---++-   --++---+---
                // .... |   || ...  ||   | ....
                //   ---+---++-   --++---+---
                //        ^            ^
                //      write         read
                auto* before_begin = other_begin->prev;
                auto* after_end = other_end->next;
                end()->next = other_begin;
                other_begin->prev = end();
                begin()->prev = other_end;
                other_end->next = begin();
                write = other_end;

                // chained lists are re-formed into rings
                before_begin->next = after_end;
                after_end->prev = before_begin;
                _other.node_size_ -= chain_size;
                _other.head = after_end;
                _other.read = _other.head;
                _other.write = _other.head;
            }

            node_size_ += chain_size;
        }

        auto cache_list::fast_expand(std::size_t _size) -> std::int32_t
        {
            auto cnt { 0 };
            get_next_write();
            auto* index = end();
            if (!index->cache) {
                index->cache.reset(new cache(MIN(round_up(_size), MAX_TO_ALLOC)));
            }

            do {
                _size -= MIN((*index)->writeable_size(), _size);
                ++cnt;
                if (index->next == read) {
                    break;
                }
                index = index->next;
            } while (true);

            while (_size > 0) {
                auto alloc_size = MIN(round_up(_size), MAX_TO_ALLOC);
                auto* tmp = new node;
                tmp->cache.reset(new cache(alloc_size));
                tmp->prev = index;
                tmp->next = index->next;
                index->next->prev = tmp;
                index->next = tmp;
                index = index->next;
                ++node_size_;
                ++cnt;
            }
            return cnt;
        }

        // The legality of size is checked before use
        void cache_list::add(std::size_t _size)
        {
            auto* index = end();
            while (_size > 0) {
                auto write_size = MIN(_size, (*index)->writeable_size());
                (*index)->add(write_size);
                _size -= write_size;
                if ((*index)->full() && index->next != read) {
                    index = index->next;
                } else {
                    assert(_size == 0);
                }
            }
            write = index;
        }

        // The legality of size is checked before use
        void cache_list::drain(std::size_t _size)
        {
            auto* index = begin();
            auto need_drain { false };
            do {
                auto drain_size = MIN(_size, (*index)->readable_size());
                _size -= drain_size;
                (*index)->drain(drain_size);
                if ((*index)->empty()) {
                    (*index)->clear();
                    
                    if (index != end()) {
                        need_drain = true;
                        index = index->next;
                    }
                }
            } while (_size != 0 && index != end());

            if (_size > 0) {
                assert(_size <= (*index)->readable_size());
                (*index)->drain(_size);
                _size = 0;
            }

            /**
             * @brief If the length of chain bigger than MAX_CHINA_SIZE,
             *   cleaning will begin.
             **/
            if (need_drain && node_size_ > MAX_CHINA_SIZE) {
                while (begin()->next != index) {
                    auto* tmp = begin()->next;
                    tmp->next->prev = begin();
                    begin()->next = tmp->next;
                    delete tmp;
                    --node_size_;
                }
            }
            read = index;
        }

        void cache_list::reset()
        {
            while (head->next != head) {
                auto* tmp = head->next;
                tmp->next->prev = head;
                head->next = tmp->next;
                delete tmp;
            }
            if (head->cache) {
                (*head)->clear();
            }
            node_size_ = 1;
            read = head;
            write = head;
        }

#ifdef HARE_DEBUG
        void cache_list::print_status(const std::string& _status) const
        {
            MSG_TRACE("[{}] list total length: {:}", _status, node_size_);

            auto* index = begin();
            do {
                /// | (RW|R|W|E|N) ... | [<-(w_itre|r_iter)]
                const auto* mark = !index->cache ? "(N)" : 
                    (*index)->readable_size() > 0 && (*index)->writeable_size() > 0 ? "(RW)" :
                    (*index)->readable_size() > 0 ? "(R)":
                    (*index)->writeable_size() > 0 ? "(W)" : "(E)";

                    MSG_TRACE("|{:4} {} {} {}|{}",
                        mark,
                        !index->cache ? 0 : (*index)->readable_size(),
                        !index->cache ? 0 : (*index)->writeable_size(),
                        !index->cache ? 0 : (*index)->capacity(),
                        index == read && index == write ? " <- w_iter|r_iter" :
                            index == read ? " <- r_iter" :
                            index == write ? " <- w_iter" : ""
                        );
                index = index->next;
            } while (index != begin());

            MSG_TRACE("{:^16}", "*");
        }
#endif

    } // namespace detail

    HARE_IMPL_DEFAULT(buffer,
        detail::cache_list cache_chain_ {};
        std::size_t total_len_ { 0 };
        std::size_t max_read_ { HARE_MAX_READ_DEFAULT };
    )

    buffer::buffer(std::size_t _max_read)
        : impl_(new buffer_impl)
    { d_ptr(impl_)->max_read_ = _max_read; }
    buffer::~buffer()
    { delete impl_; }
    
    auto buffer::size() const -> std::size_t
    { return d_ptr(impl_)->total_len_; }
    void buffer::set_max_read(std::size_t _max_read)
    { d_ptr(impl_)->max_read_ = _max_read; }

    auto buffer::chain_size() const -> std::size_t
    { return d_ptr(impl_)->cache_chain_.size(); }

    void buffer::clear_all()
    {
        d_ptr(impl_)->cache_chain_.reset();
        d_ptr(impl_)->total_len_ = 0;
    }

    void buffer::skip(std::size_t _size)
    {
        if (_size >= d_ptr(impl_)->total_len_) {
            clear_all();
        } else {
            d_ptr(impl_)->total_len_ -= _size;
            d_ptr(impl_)->cache_chain_.drain(_size);
        }
    }

    auto buffer::begin() -> iterator
    { 
        return iterator(
            new buffer_iterator_impl(&d_ptr(impl_)->cache_chain_)); 
    }

    auto buffer::end() -> iterator
    { 
        return iterator(
            new buffer_iterator_impl(
                &d_ptr(impl_)->cache_chain_, d_ptr(impl_)->cache_chain_.end())); 
    }

    auto buffer::find(const char* _begin, std::size_t _size) -> iterator
    {
        if (_size > d_ptr(impl_)->total_len_ ) {
            return end();
        }
        auto next_val = detail::get_next(_begin, _size);
        auto i { begin() };
        auto j { 0 };
        while (i != end() && (j == -1 || (std::size_t)j < _size)) {
            if (j == -1 || (*i) == _begin[j]) {
                ++i, ++j;
            } else {
                j = next_val[j];
            }
        }

        if (j >= 0 && (std::size_t)j >= _size) {
            return i;
        }
        return end();
    }

    void buffer::append(buffer& _other)
    {
        if (d_ptr(_other.impl_)->total_len_ == 0 || &_other == this) {
            return;
        }

#ifdef HARE_DEBUG
        d_ptr(impl_)->cache_chain_.print_status("before append buffer");
        d_ptr(_other.impl_)->cache_chain_.print_status("before append other buffer");
#endif

        if (d_ptr(impl_)->total_len_ == 0) {
            d_ptr(impl_)->cache_chain_.swap(d_ptr(_other.impl_)->cache_chain_);
        } else {
            d_ptr(impl_)->cache_chain_.append(d_ptr(_other.impl_)->cache_chain_);
        }

#ifdef HARE_DEBUG
        d_ptr(impl_)->cache_chain_.print_status("after append buffer");
        d_ptr(_other.impl_)->cache_chain_.print_status("after append other buffer");
#endif

        d_ptr(impl_)->total_len_ += d_ptr(_other.impl_)->total_len_;
        d_ptr(_other.impl_)->total_len_ = 0;
    }

    auto buffer::add(const void* _bytes, std::size_t _size) -> bool
    {
        if (d_ptr(impl_)->total_len_ + _size > MAX_SIZE) {
            return false;
        }

        d_ptr(impl_)->total_len_ += _size;
        d_ptr(impl_)->cache_chain_.check_size(_size);
        d_ptr(impl_)->cache_chain_.end()->cache->append(
            static_cast<const char*>(_bytes),
            static_cast<const char*>(_bytes) + _size
        );

#ifdef HARE_DEBUG
        d_ptr(impl_)->cache_chain_.print_status("after add");
#endif
        return true;
    }

    auto buffer::remove(void* _buffer, std::size_t _length) -> std::size_t
    {
        using hare::util::detail::make_checked;

        if (_length == 0 || d_ptr(impl_)->total_len_ == 0) {
            return 0;
        }

        if (_length > d_ptr(impl_)->total_len_) {
            _length = d_ptr(impl_)->total_len_;
        }

        auto* curr { d_ptr(impl_)->cache_chain_.begin() };
        std::size_t total { 0 };
        auto* dest = static_cast<char*>(_buffer);
        while (total < _length) {
            auto copy_len = MIN(_length - total, (*curr)->readable_size());
            std::uninitialized_copy_n(dest + total, copy_len, 
                make_checked((*curr)->readable(), copy_len));
            total += copy_len;
            curr = curr->next;
        }

        d_ptr(impl_)->total_len_ -= _length;
        d_ptr(impl_)->cache_chain_.drain(_length);

#ifdef HARE_DEBUG
        d_ptr(impl_)->cache_chain_.print_status("after remove");
#endif
        return _length;
    }

    auto buffer::read(util_socket_t _fd, std::size_t _howmuch) -> std::size_t
    {
        auto readable = socket_op::get_bytes_readable_on_socket(_fd);
        if (readable == 0) {
            readable = d_ptr(impl_)->max_read_;
        }
        if (_howmuch == 0 || _howmuch > readable) {
            _howmuch = readable;
        } else {
            readable = _howmuch;
        }

#ifdef HARE_DEBUG
        d_ptr(impl_)->cache_chain_.print_status("before read");
#endif

#ifdef USE_IOVEC_IMPL
        auto block_size = d_ptr(impl_)->cache_chain_.fast_expand(readable);
        std::vector<IOV_TYPE> vecs(block_size);
        auto* block = d_ptr(impl_)->cache_chain_.end();

        for (auto i = 0; i < block_size; ++i, block = block->next) {
            vecs[i].IOV_PTR_FIELD = (*block)->writeable();
            vecs[i].IOV_LEN_FIELD = (*block)->writeable_size();
        }

        {
            std::int64_t actual {};
#ifdef H_OS_WIN
            {
                DWORD bytes_read {};
                DWORD flags { 0 };
                if (::WSARecv(_fd, vecs.data(), block_size, &bytes_read, &flags, nullptr, nullptr) != 0) {
                    /* The read failed. It might be a close,
                    * or it might be an error. */
                    if (::WSAGetLastError() == WSAECONNABORTED) {
                        actual = 0;
                    } else {
                        actual = -1;
                    }
                } else {
                    readable = bytes_read;
                }
            }
#else
            actual = ::readv(_fd, vecs.data(), block_size);
            readable = actual > 0 ? actual : 0;
#endif
            if (actual <= 0) {
                return (0);
            }
        }

        d_ptr(impl_)->cache_chain_.add(readable);
        d_ptr(impl_)->total_len_ += readable;
        
#else
#error "cannot use IOVEC."
#endif

#ifdef HARE_DEBUG
        d_ptr(impl_)->cache_chain_.print_status("after read");
#endif

        return readable;
    }

    auto buffer::write(util_socket_t _fd, std::size_t _howmuch) -> std::size_t
    {
        std::int64_t write_n {};
        auto total = _howmuch == 0 ? d_ptr(impl_)->total_len_ : _howmuch;

        if (total > d_ptr(impl_)->total_len_) {
            total = d_ptr(impl_)->total_len_;
        }

#ifdef HARE_DEBUG
        d_ptr(impl_)->cache_chain_.print_status("before write");
#endif

#ifdef USE_IOVEC_IMPL
        std::array<IOV_TYPE, NUM_WRITE_IOVEC> iov {};
        auto* curr { d_ptr(impl_)->cache_chain_.begin() };
        auto write_i { 0 };

        while (write_i < NUM_WRITE_IOVEC && total > 0) {
            iov[write_i].IOV_PTR_FIELD = (*curr)->readable();
            if (total > (*curr)->readable_size()) {
                /* XXXcould be problematic when windows supports mmap*/
                iov[write_i++].IOV_LEN_FIELD = static_cast<IOV_LEN_TYPE>((*curr)->readable_size());
                total -= (*curr)->readable_size();
                curr = curr->next;
            } else {
                /* XXXcould be problematic when windows supports mmap*/
                iov[write_i++].IOV_LEN_FIELD = static_cast<IOV_LEN_TYPE>(total);
                total = 0;
            }
        }

        if (write_i == 0) {
            return (0);
        }

        {
            std::int64_t actual {};
#ifdef H_OS_WIN
            {
                DWORD bytes_sent {};
                if (::WSASend(_fd, iov.data(), write_i, &bytes_sent, 0, nullptr, nullptr)) {
                    actual = -1;
                } else {
                    write_n = bytes_sent;
                }
            }
#else
            actual = ::writev(_fd, iov.data(), write_i);
            write_n = actual > 0 ? actual : 0;
#endif
            if (actual <= 0) {
                return (0);
            }
        }
        
        d_ptr(impl_)->total_len_ -= write_n;
        d_ptr(impl_)->cache_chain_.drain(write_n);
        
#else
#error "cannot use IOVEC."
#endif

#ifdef HARE_DEBUG
        d_ptr(impl_)->cache_chain_.print_status("after write");
#endif

        return write_n;
    }

    void buffer::move(buffer& _other) noexcept
    {
        d_ptr(impl_)->cache_chain_.swap(d_ptr(_other.impl_)->cache_chain_);
        std::swap(d_ptr(impl_)->total_len_, d_ptr(_other.impl_)->total_len_);
        std::swap(d_ptr(impl_)->max_read_, d_ptr(_other.impl_)->max_read_);
    }

} // namespace net
} // namespace hare