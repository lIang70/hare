#include "hare/net/socket_op.h"
#include <hare/base/logging.h>
#include <hare/net/buffer.h>

#include <limits>
#include <list>
#include <memory>

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
        //!
        //! @brief The block of buffer.
        //! @code
        //! +----------------+-------------+------------------+
        //! | misalign bytes |  off bytes  |  residual bytes  |
        //! |                |  (CONTENT)  |                  |
        //! +----------------+-------------+------------------+
        //! |                |             |                  |
        //! @endcode
        class Block {
        public:
            std::size_t buffer_len_ { 0 };
            std::size_t misalign_ { 0 };
            std::size_t off_ { 0 };
            unsigned char* bytes_ { nullptr };

            enum : std::size_t {
                MIN_SIZE = 4096,
                MAX_SIZE = std::numeric_limits<std::size_t>::max()
            };

            explicit Block(std::size_t size);
            ~Block();

            inline void* writable() const { return bytes_ + off_ + misalign_; }
            inline std::size_t writableSize() const { return buffer_len_ - off_ - misalign_; }
            inline void* readable() const { return bytes_ + misalign_; }
            inline std::size_t readableSize() const { return off_; }
            inline bool isFull() const { return off_ + misalign_ == buffer_len_; }
            inline bool isEmpty() const { return off_ == 0; }

            void clear();
            void write(void* bytes, std::size_t size);
            void drain(std::size_t size);
            bool realign(std::size_t size);
        };

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

        void Block::write(void* bytes, std::size_t size)
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

        bool Block::realign(std::size_t size)
        {
            if (buffer_len_ - off_ > size && off_ < buffer_len_ / 2 && off_ <= MAX_TO_REALIGN_IN_EXPAND) {
                memmove(bytes_, bytes_ + misalign_, off_);
                misalign_ = 0;
                return true;
            }
            return false;
        }
    } // namespace detail

    using UBlock = std::unique_ptr<detail::Block>;
    using BlockList = std::list<UBlock>;

    /*
        Buffer part.
    */

    class Buffer::Data {
    public:
        //! @code
        //! +-------++-------++-------++-------++-------++-------+
        //! | block || block || block || block || block || block |
        //! +-------++-------++-------++-------++-------++-------+
        //!                            |
        //!                            write_iter
        //! @endcode
        BlockList block_chain_ {};
        BlockList::iterator write_iter_ { block_chain_.begin() };

        BlockList::iterator insertBlock(detail::Block* block)
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
    };

    Buffer::Buffer(std::size_t max_read)
        : d_(new Data)
        , max_read_(max_read)
    {
    }

    Buffer::~Buffer()
    {
        clearAll();
        delete d_;
    }

    void Buffer::clearAll()
    {
        drain(total_len_);
    }

    bool Buffer::add(void* bytes, std::size_t size)
    {
        if (total_len_ + size > detail::Block::MAX_SIZE) {
            return false;
        }

        decltype(d_->write_iter_) curr {};
        std::size_t remain {};
        if (d_->write_iter_ == d_->block_chain_.end()) {
            curr = d_->insertBlock(new detail::Block(size));
            if (curr == d_->block_chain_.end())
                return false;
        } else {
            curr = d_->write_iter_;
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

        curr = d_->insertBlock(new detail::Block(size - remain));
        if (curr == d_->block_chain_.end())
            return false;

        if (remain > 0)
            curr_block->write(bytes, remain);
        (*curr)->write((unsigned char*)bytes + remain, size - remain);
        total_len_ += size;

    out:
        d_->write_iter_ = curr;
        return true;
    }

    std::size_t Buffer::remove(void* buffer, std::size_t length)
    {
        std::size_t n = 0;
        n = copyout(buffer, length);
        if (n > 0) {
            return drain(length) ? n : 0;
        }
        return n;
    }

    int64_t Buffer::read(util_socket_t fd, int64_t howmuch)
    {
        auto n = socket::getBytesReadableOnSocket(fd);
        auto nvecs = 2;
        if (n <= 0 || n > max_read_)
            n = max_read_;
        if (howmuch < 0 || howmuch > n)
            howmuch = n;

#ifdef USE_IOVEC_IMPL
        if (!expandFast(howmuch)) {
            return -1;
        } else {
            IOV_TYPE vecs[nvecs];

            auto iter = d_->write_iter_;
            for (auto i = 0; i < nvecs && iter != d_->block_chain_.end(); ++i, ++iter) {
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
        auto iter = d_->write_iter_;
        for (auto i = 0; i < nvecs && iter != d_->block_chain_.end(); ++i, ++iter) {
            auto space = (*iter)->writableSize();
            if (space > detail::Block::MAX_SIZE)
                space = detail::Block::MAX_SIZE;
            if (space < remaining) {
                (*iter)->off_ += space;
                remaining -= space;
            } else {
                (*iter)->off_ += remaining;
                d_->write_iter_ = iter;
                break;
            }
        }
#else
        // FIXME : Not use iovec.
#endif
        total_len_ += n;

        return n;
    }

    int64_t Buffer::write(util_socket_t fd, int64_t howmuch)
    {
        auto n = -1;

        if (howmuch < 0 || howmuch > total_len_)
            howmuch = total_len_;

        if (howmuch > 0) {
#ifdef USE_IOVEC_IMPL
            IOV_TYPE iov[NUM_WRITE_IOVEC];
            int i = 0;
            auto curr = d_->block_chain_.begin();

            while (curr != d_->block_chain_.end() && i < NUM_WRITE_IOVEC && howmuch) {
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

    std::size_t Buffer::copyout(void* buffer, std::size_t length)
    {
        if (length == 0)
            return length;

        if (length > total_len_)
            length = total_len_;

        auto _buffer = (unsigned char*)buffer;
        auto block = d_->block_chain_.begin();
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
            d_->block_chain_.clear();
            d_->write_iter_ = d_->block_chain_.end();
            total_len_ = 0;
        } else {
            auto block = d_->block_chain_.begin();
            auto remaining = size;
            total_len_ -= size;
            for (; remaining >= (*block)->readableSize();) {
                remaining -= (*block)->readableSize();
                if (block == d_->write_iter_)
                    ++d_->write_iter_;
                ++block;
                d_->block_chain_.pop_front();
            }

            (*block)->drain(remaining);
        }
        return true;
    }

    bool Buffer::expandFast(int64_t howmuch)
    {
        decltype(d_->write_iter_) curr {};

        if (d_->write_iter_ == d_->block_chain_.end()) {
            curr = d_->insertBlock(new detail::Block(howmuch));
            if (curr == d_->block_chain_.end())
                return false;
            else
                return true;
        }

        curr = d_->write_iter_;
        auto remain = (*curr)->writableSize();
        curr = d_->insertBlock(new detail::Block(howmuch - remain));
        if (curr == d_->block_chain_.end())
            return false;
        else
            return true;
    }

} // namespace net
} // namespace hare
