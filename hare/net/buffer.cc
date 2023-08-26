#include "base/fwd-inl.h"
#include "net/buffer-inl.h"
#include "socket_op.h"
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

        auto Cache::Realign(std::size_t _size) -> bool
        {
            auto offset = ReadableSize();
            if (WriteableSize() >= _size) {
                return true;
            } else if (WriteableSize() + misalign_ > _size && offset <= MAX_TO_REALIGN) {
                ::memmove(Begin(), Readable(), offset);
                Set(Begin(), offset);
                misalign_ = 0;
                return true;
            }
            return false;
        }

        void CacheList::CheckSize(std::size_t _size)
        {
            if (!write->cache) {
                write->cache.reset(new Cache(round_up(_size)));
            } else if (!(*write)->Realign(_size)) {
                GetNextWrite();
                if (!write->cache || (*write)->size() < _size) {
                    write->cache.reset(new Cache(round_up(_size)));
                }
            }
        }

        void CacheList::Append(CacheList& _other)
        {
            auto* other_begin = _other.Begin();
            auto* other_end = _other.End();
            auto chain_size = 1;

            while (other_begin != other_end) {
                ++chain_size;
                other_begin = other_begin->next;
            }
            other_begin = _other.Begin();

            if (other_begin->prev == other_end) {
                //   ---+---++---+---
                // .... |   ||   | ....
                //   ---+---++---+---
                //        ^    ^
                //      write read
                End()->next = other_begin;
                other_begin->prev = End();
                Begin()->prev = other_end;
                other_end->next = Begin();
                write = other_end;

                // reset other
                _other.node_size_ = 1;
                _other.head = new detail::CacheList::Node;
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
                End()->next = other_begin;
                other_begin->prev = End();
                Begin()->prev = other_end;
                other_end->next = Begin();
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

        auto CacheList::FastExpand(std::size_t _size) -> std::int32_t
        {
            auto cnt { 0 };
            GetNextWrite();
            auto* index = End();
            if (!index->cache) {
                index->cache.reset(new Cache(Min(round_up(_size), MAX_TO_ALLOC)));
            }

            do {
                _size -= Min((*index)->WriteableSize(), _size);
                ++cnt;
                if (index->next == read) {
                    break;
                }
                index = index->next;
            } while (true);

            while (_size > 0) {
                auto alloc_size = Min(round_up(_size), MAX_TO_ALLOC);
                auto* tmp = new Node;
                tmp->cache.reset(new Cache(alloc_size));
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
        void CacheList::Add(std::size_t _size)
        {
            auto* index = End();
            while (_size > 0) {
                auto write_size = Min(_size, (*index)->WriteableSize());
                (*index)->Add(write_size);
                _size -= write_size;
                if ((*index)->Full() && index->next != read) {
                    index = index->next;
                } else {
                    assert(_size == 0);
                }
            }
            write = index;
        }

        // The legality of size is checked before use
        void CacheList::Drain(std::size_t _size)
        {
            auto* index = Begin();
            auto need_drain { false };
            do {
                auto drain_size = Min(_size, (*index)->ReadableSize());
                _size -= drain_size;
                (*index)->Drain(drain_size);
                if ((*index)->Empty()) {
                    (*index)->Clear();
                    
                    if (index != End()) {
                        need_drain = true;
                        index = index->next;
                    }
                }
            } while (_size != 0 && index != End());

            if (_size > 0) {
                assert(_size <= (*index)->ReadableSize());
                (*index)->Drain(_size);
                _size = 0;
            }

            /**
             * @brief If the length of chain bigger than MAX_CHINA_SIZE,
             *   cleaning will begin.
             **/
            if (need_drain && node_size_ > MAX_CHINA_SIZE) {
                while (Begin()->next != index) {
                    auto* tmp = Begin()->next;
                    tmp->next->prev = Begin();
                    Begin()->next = tmp->next;
                    delete tmp;
                    --node_size_;
                }
            }
            read = index;
        }

        void CacheList::Reset()
        {
            while (head->next != head) {
                auto* tmp = head->next;
                tmp->next->prev = head;
                head->next = tmp->next;
                delete tmp;
            }
            if (head->cache) {
                (*head)->Clear();
            }
            node_size_ = 1;
            read = head;
            write = head;
        }

#ifdef HARE_DEBUG
        void CacheList::PrintStatus(const std::string& _status) const
        {
            HARE_INTERNAL_TRACE("[{}] list total length: {:}", _status, node_size_);

            auto* index = Begin();
            do {
                /// | (RW|R|W|E|N) ... | [<-(w_itre|r_iter)]
                const auto* mark = !index->cache ? "(N)" : 
                    (*index)->ReadableSize() > 0 && (*index)->WriteableSize() > 0 ? "(RW)" :
                    (*index)->ReadableSize() > 0 ? "(R)":
                    (*index)->WriteableSize() > 0 ? "(W)" : "(E)";

                    HARE_INTERNAL_TRACE("|{:4} {} {} {}|{}",
                        mark,
                        !index->cache ? 0 : (*index)->ReadableSize(),
                        !index->cache ? 0 : (*index)->WriteableSize(),
                        !index->cache ? 0 : (*index)->capacity(),
                        index == read && index == write ? " <- w_iter|r_iter" :
                            index == read ? " <- r_iter" :
                            index == write ? " <- w_iter" : ""
                        );
                index = index->next;
            } while (index != Begin());

            HARE_INTERNAL_TRACE("{:^16}", "*");
        }
#endif

    } // namespace detail

    HARE_IMPL_DEFAULT(
        Buffer,
        detail::CacheList cache_chain {};
        std::size_t total_len { 0 };
        std::size_t max_read { HARE_MAX_READ_DEFAULT };
    )

    Buffer::Buffer(std::size_t _max_read)
        : impl_(new BufferImpl)
    { IMPL->max_read = _max_read; }
    Buffer::~Buffer()
    { delete impl_; }
    
    auto Buffer::Size() const -> std::size_t
    { return IMPL->total_len; }
    void Buffer::SetMaxRead(std::size_t _max_read)
    { IMPL->max_read = _max_read; }

    auto Buffer::ChainSize() const -> std::size_t
    { return IMPL->cache_chain.Size(); }

    void Buffer::ClearAll()
    {
        IMPL->cache_chain.Reset();
        IMPL->total_len = 0;
    }

    void Buffer::Skip(std::size_t _size)
    {
        if (_size >= IMPL->total_len) {
            ClearAll();
        } else {
            IMPL->total_len -= _size;
            IMPL->cache_chain.Drain(_size);
        }
    }

    auto Buffer::Begin() -> Iterator
    { 
        return Iterator(
            new buffer_iterator_impl(&IMPL->cache_chain)); 
    }

    auto Buffer::End() -> Iterator
    { 
        return Iterator(
            new buffer_iterator_impl(
                &IMPL->cache_chain, IMPL->cache_chain.End())); 
    }

    auto Buffer::Find(const char* _begin, std::size_t _size) -> Iterator
    {
        if (_size > IMPL->total_len ) {
            return End();
        }
        auto next_val = detail::get_next(_begin, _size);
        auto i { Begin() };
        auto j { 0 };
        while (i != End() && (j == -1 || (std::size_t)j < _size)) {
            if (j == -1 || (*i) == _begin[j]) {
                ++i, ++j;
            } else {
                j = next_val[j];
            }
        }

        if (j >= 0 && (std::size_t)j >= _size) {
            return i;
        }
        return End();
    }

    void Buffer::Append(Buffer& _other)
    {
        if (d_ptr(_other.impl_)->total_len == 0 || &_other == this) {
            return;
        }

#ifdef HARE_DEBUG
        IMPL->cache_chain.PrintStatus("before append buffer");
        d_ptr(_other.impl_)->cache_chain.PrintStatus("before append other buffer");
#endif

        if (IMPL->total_len == 0) {
            IMPL->cache_chain.Swap(d_ptr(_other.impl_)->cache_chain);
        } else {
            IMPL->cache_chain.Append(d_ptr(_other.impl_)->cache_chain);
        }

#ifdef HARE_DEBUG
        IMPL->cache_chain.PrintStatus("after append buffer");
        d_ptr(_other.impl_)->cache_chain.PrintStatus("after append other buffer");
#endif

        IMPL->total_len += d_ptr(_other.impl_)->total_len;
        d_ptr(_other.impl_)->total_len = 0;
    }

    auto Buffer::Add(const void* _bytes, std::size_t _size) -> bool
    {
        if (IMPL->total_len + _size > MAX_SIZE) {
            return false;
        }

        IMPL->total_len += _size;
        IMPL->cache_chain.CheckSize(_size);
        IMPL->cache_chain.End()->cache->Append(
            static_cast<const char*>(_bytes),
            static_cast<const char*>(_bytes) + _size
        );

#ifdef HARE_DEBUG
        IMPL->cache_chain.PrintStatus("after add");
#endif
        return true;
    }

    auto Buffer::Remove(void* _buffer, std::size_t _length) -> std::size_t
    {
        using hare::util::detail::MakeChecked;

        if (_length == 0 || IMPL->total_len == 0) {
            return 0;
        }

        if (_length > IMPL->total_len) {
            _length = IMPL->total_len;
        }

        auto* curr { IMPL->cache_chain.Begin() };
        std::size_t total { 0 };
        auto* dest = static_cast<char*>(_buffer);
        while (total < _length) {
            auto copy_len = Min(_length - total, (*curr)->ReadableSize());
            std::uninitialized_copy_n(dest + total, copy_len, 
                MakeChecked((*curr)->Readable(), copy_len));
            total += copy_len;
            curr = curr->next;
        }

        IMPL->total_len -= _length;
        IMPL->cache_chain.Drain(_length);

#ifdef HARE_DEBUG
        IMPL->cache_chain.PrintStatus("after remove");
#endif
        return _length;
    }

    auto Buffer::Read(util_socket_t _fd, std::size_t _howmuch) -> std::size_t
    {
        auto readable = socket_op::GetBytesReadableOnSocket(_fd);
        if (readable == 0) {
            readable = IMPL->max_read;
        }
        if (_howmuch == 0 || _howmuch > readable) {
            _howmuch = readable;
        } else {
            readable = _howmuch;
        }

#ifdef HARE_DEBUG
        IMPL->cache_chain.PrintStatus("before read");
#endif

#ifdef USE_IOVEC_IMPL
        auto block_size = IMPL->cache_chain.FastExpand(readable);
        std::vector<IOV_TYPE> vecs(block_size);
        auto* block = IMPL->cache_chain.End();

        for (auto i = 0; i < block_size; ++i, block = block->next) {
            vecs[i].IOV_PTR_FIELD = (*block)->Writeable();
            vecs[i].IOV_LEN_FIELD = (*block)->WriteableSize();
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

        IMPL->cache_chain.Add(readable);
        IMPL->total_len += readable;
        
#else
#error "cannot use IOVEC."
#endif

#ifdef HARE_DEBUG
        IMPL->cache_chain.PrintStatus("after read");
#endif

        return readable;
    }

    auto Buffer::Write(util_socket_t _fd, std::size_t _howmuch) -> std::size_t
    {
        std::size_t write_n {};
        auto total = _howmuch == 0 ? IMPL->total_len : _howmuch;

        if (total > IMPL->total_len) {
            total = IMPL->total_len;
        }

#ifdef HARE_DEBUG
        IMPL->cache_chain.PrintStatus("before write");
#endif

#ifdef USE_IOVEC_IMPL
        std::array<IOV_TYPE, NUM_WRITE_IOVEC> iov {};
        auto* curr { IMPL->cache_chain.Begin() };
        auto write_i { 0 };

        while (write_i < NUM_WRITE_IOVEC && total > 0) {
            iov[write_i].IOV_PTR_FIELD = (*curr)->Readable();
            if (total > (*curr)->ReadableSize()) {
                /* XXXcould be problematic when windows supports mmap*/
                iov[write_i++].IOV_LEN_FIELD = static_cast<IOV_LEN_TYPE>((*curr)->ReadableSize());
                total -= (*curr)->ReadableSize();
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
        
        IMPL->total_len -= write_n;
        IMPL->cache_chain.Drain(write_n);
        
#else
#error "cannot use IOVEC."
#endif

#ifdef HARE_DEBUG
        IMPL->cache_chain.PrintStatus("after write");
#endif

        return write_n;
    }

    void Buffer::Move(Buffer& _other) noexcept
    {
        IMPL->cache_chain.Swap(d_ptr(_other.impl_)->cache_chain);
        std::swap(IMPL->total_len, d_ptr(_other.impl_)->total_len);
        std::swap(IMPL->max_read, d_ptr(_other.impl_)->max_read);
    }

} // namespace net
} // namespace hare