#ifndef _HARE_NET_IO_POOL_H_
#define _HARE_NET_IO_POOL_H_

#include <hare/base/io/cycle.h>
#include <hare/base/thread/thread.h>

#include <utility>
#include <vector>

namespace hare {
namespace net {

    class io_pool : public non_copyable {
        using cycle_list = std::vector<ptr<io::cycle>>;
        using thread_list = std::vector<ptr<thread>>;

        std::string name_ {};
        bool is_running_ { false };
        int32_t last_ { -1 };
        int32_t thread_nbr_ { 0 };
        cycle_list io_cycles_ {};
        thread_list cycle_thread_ {};

    public:
        explicit io_pool(std::string _name)
            : name_(std::move(_name))
        {
        }
        ~io_pool();

        inline auto name() const -> const std::string& { return name_; }
        inline auto is_running() const -> bool { return is_running_; }
        inline auto get_all() const -> cycle_list { return io_cycles_; }

        auto start(io::cycle::REACTOR_TYPE _type, int32_t _thread_nbr) -> bool;
        void stop();

        /**
         * @brief Valid after calling start().
         *   round-robin
         */
        auto get_next() -> io::cycle*;

        /**
         * @brief With the same hash code, it will always return the same EventLoop
         */
        auto get_by_hash(size_t _hash_code) -> io::cycle*;

    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_IO_POOL_H_