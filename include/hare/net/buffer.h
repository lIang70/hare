#ifndef _HARE_NET_CORE_BUFFER_H_
#define _HARE_NET_CORE_BUFFER_H_

#include <hare/base/detail/non_copyable.h>
#include <hare/base/util.h>

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
        explicit Buffer(std::size_t max_read = MAX_READ_DEFAULT);
        ~Buffer();

        inline std::size_t length() { return total_len_; }
        inline void setMaxRead(std::size_t max_read) { max_read_ = max_read; }

        void clearAll();

        bool add(void* bytes, std::size_t size);

        std::size_t remove(void* buffer, std::size_t length);

        int64_t read(util_socket_t fd, int64_t howmuch);
        int64_t write(util_socket_t fd, int64_t howmuch = -1);

    private:
        std::size_t copyout(void* buffer, std::size_t length);
        bool drain(std::size_t size);

        bool expandFast(int64_t howmuch);
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_CORE_BUFFER_H_