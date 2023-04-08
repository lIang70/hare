#ifndef _HARE_NET_CORE_BUFFER_H_
#define _HARE_NET_CORE_BUFFER_H_

#include <hare/base/util/non_copyable.h>

#include <cstdint>

namespace hare {
namespace net {

    class Buffer : public NonCopyable {
    public:
        static const std::size_t MAX_READ_DEFAULT = 4096;

    private:
        class Data;
        Data* d_ { nullptr };

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
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_CORE_BUFFER_H_