#include "hare/net/io_pool.h"

#include <hare/base/logging.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/net/session.h>

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

        items_.resize(_thread_nbr);

        LOG_TRACE() << "start io_pool.";
        for (auto i = 0; i < _thread_nbr; ++i) {
            items_[i].thread = std::make_shared<thread>([=] {
                items_[i].cycle = std::make_shared<io::cycle>(_type);
                items_[i].cycle->loop();
                items_[i].cycle.reset();
            },
                name_ + std::to_string(i));
            items_[i].thread->start();
        }

        is_running_ = true;
        thread_nbr_ = _thread_nbr;
        return true;
    }

    void io_pool::stop()
    {
        LOG_TRACE() << "stop io_pool.";
        for (auto& item : items_) {
            item.cycle->run_in_cycle([=]() mutable {
                for (const auto& session : item.sessions) {
                    session.second->force_close();
                }
                item.sessions.clear();
                item.cycle->exit();
            });
        }

        for (auto& item : items_) {
            item.thread->join();
        }
        is_running_ = false;
        thread_nbr_ = 0;
        items_.clear();
    }

    auto io_pool::get_next() -> pool_item
    {
        if (!is_running()) {
            return {};
        }

        auto index = last_++;
        last_ %= thread_nbr_;
        return items_[index];
    }

    auto io_pool::get_by_hash(size_t _hash_code) -> pool_item
    {
        return items_[_hash_code % thread_nbr_];
    }

    void io_pool::add_session(thread::id _tid, util_socket_t _fd, const ptr<session>& _session)
    {
        for (auto& item : items_) {
            if (item.thread->tid() == _tid) {
                HARE_ASSERT(item.sessions.find(_fd) == item.sessions.end(), "");
                item.sessions.insert(std::make_pair(_fd, _session));
                break;
            }
        }
    }

    void io_pool::del_session(thread::id _tid, util_socket_t _fd)
    {
        for (auto& item : items_) {
            if (item.thread->tid() == _tid) {
                item.cycle->run_in_cycle([&]() mutable {
                    HARE_ASSERT(item.sessions.find(_fd) != item.sessions.end(), "");
                    item.sessions.erase(_fd);
                });
                break;
            }
        }
    }

} // namespace net
} // namespace hare