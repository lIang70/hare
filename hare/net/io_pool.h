#ifndef _HARE_NET_IO_POOL_H_
#define _HARE_NET_IO_POOL_H_

#include <hare/base/io/cycle.h>

#include <map>
#include <vector>
#include <thread>

namespace hare {
namespace net {

    class session;

    struct pool_item {
        ptr<io::cycle> cycle {};
        ptr<std::thread> thread {};
        std::map<util_socket_t, ptr<session>> sessions {};
    };

    class io_pool : public util::non_copyable {

        using pool_items = std::vector<ptr<pool_item>>;

        std::string name_ {};
        bool is_running_ { false };
        int32_t last_ { 0 };
        int32_t thread_nbr_ { 0 };

        pool_items items_ {};

    public:
        explicit io_pool(std::string _name)
            : name_(std::move(_name))
        {
        }
        ~io_pool();

        inline auto name() const -> const std::string& { return name_; }
        inline auto is_running() const -> bool { return is_running_; }

        auto start(io::cycle::REACTOR_TYPE _type, int32_t _thread_nbr) -> bool;
        void stop();

        /**
         * @brief Valid after calling start().
         *   round-robin
         */
        auto get_next() -> ptr<pool_item>;

        /**
         * @brief With the same hash code, it will always return the same EventLoop
         */
        auto get_by_hash(size_t _hash_code) -> ptr<pool_item>;
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_IO_POOL_H_