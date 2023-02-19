#include "hare/net/core/buffer.h"
#include "hare/net/socket_op.h"
#include <hare/base/logging.h>

#define MAX_TO_COPY_IN_EXPAND 4096
#define MAX_TO_REALIGN_IN_EXPAND 2048

namespace hare {
namespace core {

    Buffer::Block::Block(std::size_t size)
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

    Buffer::Block::~Block()
    {
        delete[](bytes_);
    }

    void Buffer::Block::clear()
    {
        misalign_ = 0;
        off_ = 0;
    }

    void Buffer::Block::write(void* bytes, std::size_t size)
    {
        HARE_ASSERT(size <= writableSize(), "");
        memcpy(writable(), bytes, size);
        off_ += size;
    }

    void Buffer::Block::drain(std::size_t size)
    {
        HARE_ASSERT(size <= readableSize(), "");
        misalign_ += size;
        off_ -= size;
    }

    bool Buffer::Block::realign(std::size_t size)
    {
        if (buffer_len_ - off_ > size && off_ < buffer_len_ / 2 && off_ <= MAX_TO_REALIGN_IN_EXPAND) {
            memmove(bytes_, bytes_ + misalign_, off_);
            misalign_ = 0;
            return true;
        }
        return false;
    }

    /*
        Buffer part.
    */

    Buffer::~Buffer()
    {
        clearAll();
    }

    void Buffer::clearAll()
    {
        std::unique_lock<std::mutex> locker(buffer_mutex_);
        drain(total_len_);
    }

    bool Buffer::add(void* bytes, std::size_t size)
    {
        if (total_len_ + size > Block::MAX_SIZE) {
            return false;
        }

        std::unique_lock<std::mutex> locker(buffer_mutex_);
        decltype(write_iter_) curr {};
        std::size_t remain { 0 };
        if (write_iter_ == block_chain_.end()) {
            curr = insertBlock(new Block(size));
            if (curr == block_chain_.end())
                return false;
        } else {
            curr = write_iter_;
        }

        auto& curr_block = *curr;
        if (curr_block) {
            remain = curr_block->writableSize();
            if (remain >= size) {
                /* there's enough space to hold all the data in the
                 * current last chain */
                curr_block->write(bytes, size);
                total_len_ += size;
                goto out;
            } else if (curr_block->realign(size)) {
                curr_block->write(bytes, size);
                total_len_ += size;
                goto out;
            }
        } else {
            return false;
        }

        curr = insertBlock(new Block(size - remain));
        if (curr == block_chain_.end())
            return false;

        if (remain > 0)
            curr_block->write(bytes, remain);
        (*curr)->write((unsigned char*)bytes + remain, size - remain);
        total_len_ += size;

    out:
        write_iter_ = curr;
        return true;
    }

    std::size_t Buffer::remove(void* buffer, std::size_t length)
    {
        std::size_t n = 0;
        std::unique_lock<std::mutex> locker(buffer_mutex_);
        n = copyout(buffer, length);
        if (n > 0) {
            return drain(length) ? n : 0;
        }
        return n;
    }

    int64_t Buffer::read(socket_t fd, int64_t howmuch)
    {
        auto n = socket::getBytesReadableOnSocket(fd);
        auto nvecs = 2;
        if (n <= 0 || n > max_read_)
            n = max_read_;
        if (howmuch < 0 || howmuch > n)
            howmuch = n;

        std::unique_lock<std::mutex> locker(buffer_mutex_);

#ifdef USE_IOVEC_IMPL
        if (!expandFast(howmuch)) {
            return -1;
        } else {
            IOV_TYPE vecs[nvecs];

            auto iter = write_iter_;
            for (auto i = 0; i < nvecs && iter != block_chain_.end(); ++i, ++iter) {
                if (!(*iter)->isFull()) {
                    vecs[i].IOV_PTR_FIELD = (*iter)->writable();
                    vecs[i].IOV_LEN_FIELD = (*iter)->writableSize();
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
            n = ::readv(fd, vecs, nvecs);
#endif
        }
#else
        // FIXME : Not use iovec.
#endif

        if (n <= 0) {
            return (n);
        }

#ifdef USE_IOVEC_IMPL
        auto remaining = n;
        auto iter = write_iter_;
        for (auto i = 0; i < nvecs && iter != block_chain_.end(); ++i, ++iter) {
            auto space = (*iter)->writableSize();
            if (space > Block::MAX_SIZE)
                space = Block::MAX_SIZE;
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
        total_len_ += n;

        return n;
    }

    int64_t Buffer::write(socket_t fd, int64_t howmuch)
    {
        auto n = -1;

        std::unique_lock<std::mutex> locker(buffer_mutex_);
        if (howmuch < 0 || howmuch > total_len_)
            howmuch = total_len_;

        if (howmuch > 0) {
#ifdef USE_IOVEC_IMPL
            IOV_TYPE iov[NUM_WRITE_IOVEC];
            int i = 0;
            auto curr = block_chain_.begin();

            while (curr != block_chain_.end() && i < NUM_WRITE_IOVEC && howmuch) {
                iov[i].IOV_PTR_FIELD = (*curr)->readable();
                if (howmuch >= (*curr)->readableSize()) {
                    /* XXXcould be problematic when windows supports mmap*/
                    iov[i++].IOV_LEN_FIELD = (IOV_LEN_TYPE)(*curr)->readableSize();
                    howmuch -= (*curr)->readableSize();
                } else {
                    /* XXXcould be problematic when windows supports mmap*/
                    iov[i++].IOV_LEN_FIELD = (IOV_LEN_TYPE)howmuch;
                    break;
                }
                ++curr;
            }
            if (!i) {
                n = 0;
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
            n = ::writev(fd, iov, i);
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

        if (n > 0)
            drain(n);

        return (n);
    }

    Buffer::BlockList::iterator Buffer::insertBlock(Block* block)
    {
        if (!block)
            return block_chain_.end();

        if (block_chain_.empty()) {
            HARE_ASSERT(write_iter_ == block_chain_.end(), "");
            block_chain_.emplace_back(block);
            write_iter_ = block_chain_.begin();
            return write_iter_;
        } else {
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
            if (!block->isEmpty())
                ++write_iter_;
            return --iter;
        }
    }

    std::size_t Buffer::copyout(void* buffer, std::size_t length)
    {
        if (length == 0)
            return length;

        if (length > total_len_)
            length = total_len_;

        auto _buffer = (unsigned char*)buffer;
        auto block = block_chain_.begin();
        auto nread = length;

        while (length && length > (*block)->readableSize()) {
            auto copylen = (*block)->readableSize();
            memcpy(_buffer, (*block)->readable(), copylen);
            _buffer += copylen;
            length -= copylen;
            ++block;
            HARE_ASSERT((*block) || length == 0, "");
        }

        if (length) {
            HARE_ASSERT((*block), "");
            HARE_ASSERT(length <= (*block)->readableSize(), "");

            memcpy(_buffer, (*block)->readable(), length);
        }

        return nread;
    }

    bool Buffer::drain(std::size_t size)
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
                if (block == write_iter_)
                    ++write_iter_;
                ++block;
                block_chain_.pop_front();
            }

            (*block)->drain(remaining);
        }
        return true;
    }

    bool Buffer::expandFast(int64_t howmuch)
    {
        decltype(write_iter_) curr {};

        if (write_iter_ == block_chain_.end()) {
            curr = insertBlock(new Block(howmuch));
            if (curr == block_chain_.end())
                return false;
            else
                return true;
        }

        curr = write_iter_;
        auto remain = (*curr)->writableSize();
        curr = insertBlock(new Block(howmuch - remain));
        if (curr == block_chain_.end())
            return false;
        else
            return true;
    }

} // namespace core
} // namespace hare
