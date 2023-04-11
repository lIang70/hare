#include <hare/net/buffer.h>

#include "hare/net/socket_op.h"
#include <hare/base/logging.h>

#if defined(HARE__HAVE_SYS_UIO_H) || defined(H_OS_WIN32)
#define USE_IOVEC_IMPL
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

namespace hare {
namespace net {

    namespace detail {

        Block::Block(std::size_t size)
        {
            std::size_t to_alloc { 0 };
            if (size > MAX_SIZE) {
                LOG_FATAL() << "Block size overflow.";
            }

            if (size < MAX_SIZE / 2) {
                to_alloc = MIN_SIZE;
                while (to_alloc < size) {
                    to_alloc <<= 1;
                }
            } else {
                to_alloc = size;
            }

            bytes_ = new unsigned char[to_alloc];
            buffer_len_ = to_alloc;
        }

        Block::~Block()
        {
            delete[](bytes_);
        }

        void Block::clear()
        {
            misalign_ = 0;
            off_ = 0;
        }

        void Block::write(const void* bytes, std::size_t size)
        {
            HARE_ASSERT(size <= writableSize(), "");
            memcpy(writable(), bytes, size);
            off_ += size;
        }

        void Block::drain(std::size_t size)
        {
            HARE_ASSERT(size <= readableSize(), "");
            misalign_ += size;
            off_ -= size;
        }

        auto Block::realign(std::size_t size) -> bool
        {
            if (buffer_len_ - off_ > size && off_ < buffer_len_ / 2 && off_ <= MAX_TO_REALIGN_IN_EXPAND) {
                memmove(bytes_, bytes_ + misalign_, off_);
                misalign_ = 0;
                return true;
            }
            return false;
        }
    } // namespace detail

    Buffer::Buffer(std::size_t max_read)
        : max_read_(max_read)
    {
    }

    Buffer::~Buffer()
    {
        clearAll();
    }

    void Buffer::clearAll()
    {
        drain(total_len_);
    }

    auto Buffer::add(const void* bytes, std::size_t size) -> bool
    {
        if (total_len_ + size > detail::Block::MAX_SIZE) {
            return false;
        }

        decltype(write_iter_) curr {};
        std::size_t remain {};
        if (write_iter_ == block_chain_.end()) {
            curr = insertBlock(new detail::Block(size));
            if (curr == block_chain_.end()) {
                return false;
            }
        } else {
            curr = write_iter_;
        }

        auto& curr_block = *curr;
        if (curr_block) {
            remain = curr_block->writableSize();
            if (remain >= size || curr_block->realign(size)) {
                /* there's enough space to hold all the data in the
                 * current last chain */
                curr_block->write(bytes, size);
                total_len_ += size;
                goto out;
            }
        } else {
            return false;
        }

        curr = insertBlock(new detail::Block(size - remain));
        if (curr == block_chain_.end()) {
            return false;
        }

        if (remain > 0) {
            curr_block->write(bytes, remain);
        }

        (*curr)->write(static_cast<const unsigned char*>(bytes) + remain, size - remain);
        total_len_ += size;

    out:
        write_iter_ = curr;
        return true;
    }

    auto Buffer::remove(void* buffer, std::size_t length) -> std::size_t
    {
        std::size_t copy_out = copyout(buffer, length);
        if (copy_out > 0) {
            return drain(length) ? copy_out : 0;
        }
        return copy_out;
    }

    auto Buffer::read(util_socket_t target_fd, int64_t howmuch) -> int64_t
    {
        auto readable = socket::getBytesReadableOnSocket(target_fd);
        auto nvecs = 2;
        if (readable <= 0 || readable > max_read_) {
            readable = max_read_;
        }
        if (howmuch < 0 || howmuch > readable) {
            howmuch = static_cast<int64_t>(readable);
        }

#ifdef USE_IOVEC_IMPL
        if (!expandFast(howmuch)) {
            return -1;
        }

        IOV_TYPE vecs[nvecs];

        auto iter = write_iter_;
        auto index = 0;
        for (; index < nvecs && iter != block_chain_.end(); ++index, ++iter) {
            if (!(*iter)->isFull()) {
                vecs[index].IOV_PTR_FIELD = (*iter)->writable();
                vecs[index].IOV_LEN_FIELD = (*iter)->writableSize();
            }
        }

#ifdef H_OS_WIN32
        {
            DWORD bytes_read;
            DWORD flags = 0;
            if (::WSARecv(fd, vecs, nvecs, &bytes_read, &flags, NULL, NULL)) {
                /* The read failed. It might be a close,
                 * or it might be an error. */
                if (::WSAGetLastError() == WSAECONNABORTED)
                    n = 0;
                else
                    n = -1;
            } else
                n = bytes_read;
        }
#else
        readable = ::readv(target_fd, vecs, index);
#endif

#else
        // FIXME : Not use iovec.
#endif

        if (readable <= 0) {
            return static_cast<int64_t>(readable);
        }

#ifdef USE_IOVEC_IMPL
        auto remaining = readable;
        iter = write_iter_;
        for (auto i = 0; i < nvecs && iter != block_chain_.end(); ++i, ++iter) {
            auto space = (*iter)->writableSize();
            if (space > detail::Block::MAX_SIZE) {
                space = detail::Block::MAX_SIZE;
            }
            if (space < remaining) {
                (*iter)->off_ += space;
                remaining -= space;
            } else {
                (*iter)->off_ += remaining;
                write_iter_ = iter;
                break;
            }
        }
#else
        // FIXME : Not use iovec.
#endif
        total_len_ += readable;

        return static_cast<int64_t>(readable);
    }

    auto Buffer::write(util_socket_t target_fd, int64_t howmuch) -> int64_t
    {
        auto write_n = static_cast<ssize_t>(-1);

        if (howmuch < 0 || howmuch > total_len_) {
            howmuch = static_cast<int64_t>(total_len_);
        }

        if (howmuch > 0) {
#ifdef USE_IOVEC_IMPL
            IOV_TYPE iov[NUM_WRITE_IOVEC];
            int write_i = 0;
            auto curr = block_chain_.begin();

            while (curr != block_chain_.end() && write_i < NUM_WRITE_IOVEC && (howmuch != 0)) {
                iov[write_i].IOV_PTR_FIELD = (*curr)->readable();
                if (howmuch >= (*curr)->readableSize()) {
                    /* XXXcould be problematic when windows supports mmap*/
                    iov[write_i++].IOV_LEN_FIELD = static_cast<IOV_LEN_TYPE>((*curr)->readableSize());
                    howmuch -= static_cast<int64_t>((*curr)->readableSize());
                } else {
                    /* XXXcould be problematic when windows supports mmap*/
                    iov[write_i++].IOV_LEN_FIELD = static_cast<IOV_LEN_TYPE>(howmuch);
                    break;
                }
                ++curr;
            }
            if (write_i == 0) {
                write_n = 0;
                return (0);
            }

#ifdef H_OS_WIN32
            {
                DWORD bytesSent;
                if (::WSASend(fd, iov, i, &bytesSent, 0, NULL, NULL))
                    n = -1;
                else
                    n = bytesSent;
            }
#else
            write_n = ::writev(target_fd, iov, write_i);
#endif

#elif defined(H_OS_WIN32)
            /* XXX(nickm) Don't disable this code until we know if
             * the WSARecv code above works. */
            void* p = evbuffer_pullup(buffer, howmuch);
            EVUTIL_ASSERT(p || !howmuch);
            n = send(fd, p, howmuch, 0);
#else
            void* p = evbuffer_pullup(buffer, howmuch);
            EVUTIL_ASSERT(p || !howmuch);
            n = write(fd, p, howmuch);
#endif
        }

        if (write_n > 0) {
            drain(write_n);
        }

        return (write_n);
    }

    auto Buffer::copyout(void* buffer, std::size_t length) -> std::size_t
    {
        if (length == 0) {
            return length;
        }

        if (length > total_len_) {
            length = total_len_;
        }

        auto* _buffer = static_cast<unsigned char*>(buffer);
        auto block = block_chain_.begin();
        auto nread = length;

        while ((length != 0U) && length > (*block)->readableSize()) {
            auto copylen = (*block)->readableSize();
            memcpy(_buffer, (*block)->readable(), copylen);
            _buffer += copylen;
            length -= copylen;
            ++block;
            HARE_ASSERT((*block) || length == 0, "");
        }

        if (length != 0U) {
            HARE_ASSERT((*block), "");
            HARE_ASSERT(length <= (*block)->readableSize(), "");

            memcpy(_buffer, (*block)->readable(), length);
        }

        return nread;
    }

    auto Buffer::drain(std::size_t size) -> bool
    {
        if (size >= total_len_) {
            block_chain_.clear();
            write_iter_ = block_chain_.end();
            total_len_ = 0;
        } else {
            auto block = block_chain_.begin();
            auto remaining = size;
            total_len_ -= size;
            for (; remaining >= (*block)->readableSize();) {
                remaining -= (*block)->readableSize();
                if (block == write_iter_) {
                    ++write_iter_;
                }
                ++block;
                block_chain_.pop_front();
            }

            (*block)->drain(remaining);
        }
        return true;
    }

    auto Buffer::expandFast(int64_t howmuch) -> bool
    {
        decltype(write_iter_) curr {};

        if (write_iter_ == block_chain_.end()) {
            curr = insertBlock(new detail::Block(howmuch));
            return curr != block_chain_.end();
        }

        curr = write_iter_;
        auto remain = (*curr)->writableSize();
        curr = insertBlock(new detail::Block(howmuch - remain));
        return curr != block_chain_.end();
    }

    auto Buffer::insertBlock(detail::Block* block) -> BlockList::iterator
    {
        if (block == nullptr) {
            return block_chain_.end();
        }

        if (block_chain_.empty()) {
            HARE_ASSERT(write_iter_ == block_chain_.end(), "");
            block_chain_.emplace_back(block);
            write_iter_ = block_chain_.begin();
            return write_iter_;
        }

        for (auto iter = write_iter_; iter != block_chain_.end();) {
            if ((*iter)->isEmpty()) {
                block_chain_.erase(iter++);
            } else {
                ++iter;
            }
        }
        auto iter = write_iter_;
        ++iter;
        block_chain_.insert(iter, UBlock(block));
        if (!block->isEmpty()) {
            ++write_iter_;
        }
        return --iter;
    }

} // namespace net
} // namespace hare
