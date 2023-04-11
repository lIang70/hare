#ifndef _HARE_NET_CORE_BUFFER_H_
#define _HARE_NET_CORE_BUFFER_H_

#include <hare/base/util/non_copyable.h>

#include <limits>
#include <list>

namespace hare {
namespace net {

    class Buffer;

    namespace detail {

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
        class HARE_API Block {
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

            inline auto writable() const -> void* { return bytes_ + off_ + misalign_; }
            inline auto writableSize() const -> std::size_t { return buffer_len_ - off_ - misalign_; }
            inline auto readable() const -> void* { return bytes_ + misalign_; }
            inline auto readableSize() const -> std::size_t { return off_; }
            inline auto isFull() const -> bool { return off_ + misalign_ == buffer_len_; }
            inline auto isEmpty() const -> bool { return off_ == 0; }

            void clear();
            void write(const void* bytes, std::size_t size);
            void drain(std::size_t size);
            auto realign(std::size_t size) -> bool;

            friend class net::Buffer;
        };

    } // namespace detail

    class HARE_API Buffer : public NonCopyable {
    public:
        using UBlock = std::unique_ptr<detail::Block>;
        using BlockList = std::list<UBlock>;

        static const std::size_t MAX_READ_DEFAULT = 4096;

    private:
        /** @code
         *  +-------++-------++-------++-------++-------++-------+
         *  | block || block || block || block || block || block |
         *  +-------++-------++-------++-------++-------++-------+
         *                             |
         *                             write_iter
         *  @endcode
         */
        BlockList block_chain_ {};
        BlockList::iterator write_iter_ { block_chain_.begin() };

        std::size_t total_len_ { 0 };
        std::size_t max_read_ { MAX_READ_DEFAULT };

    public:
        using Ptr = std::shared_ptr<Buffer>;

        explicit Buffer(std::size_t max_read = MAX_READ_DEFAULT);
        ~Buffer();

        inline auto length() const -> std::size_t { return total_len_; }
        inline void setMaxRead(std::size_t max_read) { max_read_ = max_read; }

        void clearAll();

        auto add(const void* bytes, std::size_t size) -> bool;

        auto remove(void* buffer, std::size_t length) -> std::size_t;

        auto read(util_socket_t target_fd, int64_t howmuch) -> int64_t;
        auto write(util_socket_t target_fd, int64_t howmuch = -1) -> int64_t;

    private:
        auto copyout(void* buffer, std::size_t length) -> std::size_t;
        auto drain(std::size_t size) -> bool;

        auto expandFast(int64_t howmuch) -> bool;

        auto insertBlock(detail::Block* block) -> BlockList::iterator;
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_CORE_BUFFER_H_