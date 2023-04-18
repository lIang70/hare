#include "hare/net/io_pool.h"

#include <hare/base/logging.h>
#include <hare/base/util/count_down_latch.h>

namespace hare {
namespace net {

    io_pool::~io_pool()
    {
        stop();
    }

    auto io_pool::start(io::cycle::REACTOR_TYPE _type, int32_t _thread_nbr) -> bool
    {
        if (_thread_nbr == 0 || is_running()) {
            return false;
        }

        cycle_thread_.resize(_thread_nbr);
        io_cycles_.resize(_thread_nbr);

        LOG_TRACE() << "start io_pool.";
        for (auto i = 0; i < _thread_nbr; ++i) {
            cycle_thread_[i] = std::make_shared<thread>([=] {
                io_cycles_[i] = std::make_shared<io::cycle>(_type);
                io_cycles_[i]->loop();
                io_cycles_[i].reset();
            }, name_ + std::to_string(i));
            cycle_thread_[i]->start();
        }

        is_running_ = true;
        thread_nbr_ = _thread_nbr;
        return true;
    }

    void io_pool::stop()
    {
        LOG_TRACE() << "stop io_pool.";
        for (auto& cycle : io_cycles_) {
            cycle->exit();
        }

        for (auto& thr : cycle_thread_) {
            thr->join();
        }
        is_running_ = false;
        thread_nbr_ = 0;
        io_cycles_.clear();
        cycle_thread_.clear();
    }


    auto io_pool::get_next() -> io::cycle*
    {
        if (!is_running()) {
            return {};
        }

        auto index = last_++;
        last_ %= thread_nbr_;
        return io_cycles_[index].get();
    }

    auto io_pool::get_by_hash(size_t _hash_code) -> io::cycle*
    {
        return io_cycles_[_hash_code % thread_nbr_].get();
    }

} // namespace net
} // namespace hare