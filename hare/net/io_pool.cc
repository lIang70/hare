#include "hare/net/io_pool.h"
#include "hare/base/fwd-inl.h"
#include <hare/base/util/count_down_latch.h>
#include <hare/base/util/system.h>
#include <hare/net/tcp/session.h>

namespace hare {
namespace net {

    io_pool::~io_pool()
    {
        stop();
    }

    auto io_pool::start(io::cycle::REACTOR_TYPE _type, std::int32_t _thread_nbr) -> bool
    {
        if (_thread_nbr == 0 || is_running()) {
            return false;
        }

        items_.resize(_thread_nbr);

        MSG_TRACE("start io_pool.");
        for (auto i = 0; i < _thread_nbr; ++i) {
            items_[i] = std::make_shared<pool_item>();
            items_[i]->thread = std::make_shared<std::thread>([=] {
                util::set_thread_name((name_ + std::to_string(i)).c_str());
                items_[i]->cycle = std::make_shared<io::cycle>(_type);
                items_[i]->cycle->loop();
                items_[i]->cycle.reset();
            });
        }

        is_running_ = true;
        thread_nbr_ = _thread_nbr;
        return true;
    }

    void io_pool::stop()
    {
        MSG_TRACE("stop io_pool.");
        for (auto& item : items_) {
            item->cycle->run_in_cycle([=]() mutable {
                for (const auto& session : item->sessions) {
                    session.second->force_close();
                }
                item->sessions.clear();
                item->cycle->exit();
            });
        }

        for (auto& item : items_) {
            item->thread->join();
        }
        is_running_ = false;
        thread_nbr_ = 0;
        items_.clear();
    }

    auto io_pool::get_next() -> ptr<pool_item>
    {
        if (!is_running()) {
            return {};
        }

        auto index = last_++;
        last_ %= thread_nbr_;
        return items_[index];
    }

    auto io_pool::get_by_hash(std::size_t _hash_code) -> ptr<pool_item>
    {
        return items_[_hash_code % thread_nbr_];
    }

} // namespace net
} // namespace hare