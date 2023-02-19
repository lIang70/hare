#ifndef _HARE_NET_CORE_BUFFER_H_
#define _HARE_NET_CORE_BUFFER_H_

#include <hare/base/detail/non_copyable.h>
#include <hare/base/util.h>

#include <atomic>
#include <cstdint>
#include <limits>
#include <list>
#include <memory>
#include <mutex>

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

namespace hare {
namespace core {

    class Buffer : public std::enable_shared_from_this<Buffer>, public NonCopyable {
    public:
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
            friend class Buffer;
            std::size_t buffer_len_ { 0 };
            std::size_t misalign_ { 0 };
            std::size_t off_ { 0 };
            unsigned char* bytes_ { nullptr };

        public:
            enum : std::size_t {
                MIN_SIZE = 4096,
                MAX_SIZE = std::numeric_limits<std::size_t>::max()
            };

            explicit Block(std::size_t size);
            ~Block();

            inline void* writable() { return bytes_ + off_ + misalign_; }
            inline std::size_t writableSize() { return buffer_len_ - off_ - misalign_; }
            inline void* readable() { return bytes_ + misalign_; }
            inline std::size_t readableSize() { return off_; }
            inline bool isFull() { return off_ + misalign_ == buffer_len_; }
            inline bool isEmpty() { return off_ == 0; }

            void clear();
            void write(void* bytes, std::size_t size);
            void drain(std::size_t size);
            bool realign(std::size_t size);
        };

        using UBlock = std::unique_ptr<Block>;
        using BlockList = std::list<UBlock>;

        static const std::size_t MAX_READ_DEFAULT = 4096;

    private:
        //!
        //! @code
        //! +-------++-------++-------++-------++-------++-------+
        //! | block || block || block || block || block || block |
        //! +-------++-------++-------++-------++-------++-------+
        //!                            |
        //!                            write_iter
        //! @endcode
        BlockList block_chain_ {};
        BlockList::iterator write_iter_ { block_chain_.begin() };

        std::atomic<std::size_t> total_len_ { 0 };
        std::size_t max_read_ { MAX_READ_DEFAULT };

    public:
        explicit Buffer(std::size_t max_read = MAX_READ_DEFAULT)
            : max_read_(max_read)
        {
        }
        ~Buffer();

        inline std::size_t length() { return total_len_; }
        inline void setMaxRead(std::size_t max_read) { max_read_ = max_read; }

        void clearAll();

        bool add(void* bytes, std::size_t size);

        std::size_t remove(void* buffer, std::size_t length);

        int64_t read(util_socket_t fd, int64_t howmuch);
        int64_t write(util_socket_t fd, int64_t howmuch = -1);

    private:
        BlockList::iterator insertBlock(Block* block);

        std::size_t copyout(void* buffer, std::size_t length);
        bool drain(std::size_t size);

        bool expandFast(int64_t howmuch);
    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_CORE_BUFFER_H_