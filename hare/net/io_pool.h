#ifndef _HARE_NET_IO_POOL_H_
#define _HARE_NET_IO_POOL_H_

#include "base/fwd-inl.h"
#include <hare/base/io/cycle.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/base/util/system.h>

#include <map>
#include <thread>
#include <vector>

namespace hare {
namespace net {

    template <typename T>
    struct PoolItem {
        Ptr<io::Cycle> cycle {};
        Ptr<std::thread> thread {};
        std::map<util_socket_t, T> sessions {};
    };

    template <typename T>
    class IOPool : public util::NonCopyable {
        using PoolItems = std::vector<Ptr<PoolItem<T>>>;

        std::string name_ {};
        std::int32_t last_ { 0 };
        std::int32_t thread_nbr_ { 0 };
        bool is_running_ { false };

        PoolItems items_ {};

    public:
        explicit IOPool(std::string _name)
            : name_(std::move(_name))
        {
        }

        ~IOPool()
        {
            Stop();
        }

        HARE_INLINE auto name() const -> const std::string& { return name_; }
        HARE_INLINE auto is_running() const -> bool { return is_running_; }

        auto Start(io::Cycle::REACTOR_TYPE _type, std::int32_t _thread_nbr) -> bool;
        void Stop();

        /**
         * @brief Valid after calling start().
         *   round-robin
         */
        auto GetNextItem() -> Ptr<PoolItem<T>>;

        /**
         * @brief With the same hash code, it will always return the same EventLoop
         */
        auto GetItemByHash(std::size_t _hash_code) -> Ptr<PoolItem<T>>;
    };

    template <typename T>
    auto IOPool<T>::Start(io::Cycle::REACTOR_TYPE _type, std::int32_t _thread_nbr) -> bool
    {
        if (_thread_nbr == 0 || is_running()) {
            return false;
        }

        items_.resize(_thread_nbr);

        HARE_INTERNAL_TRACE("start IO Pool.");
        for (auto i = 0; i < _thread_nbr; ++i) {
            items_[i] = std::make_shared<PoolItem<T>>();
            items_[i]->thread = std::make_shared<std::thread>([=] {
                util::SetCurrentThreadName((name_ + std::to_string(i)).c_str());
                items_[i]->cycle = std::make_shared<io::Cycle>(_type);
                items_[i]->cycle->Exec();
                items_[i]->cycle.reset();
            });
        }

        is_running_ = true;
        thread_nbr_ = _thread_nbr;
        return true;
    }

    template <typename T>
    void IOPool<T>::Stop()
    {
        HARE_INTERNAL_TRACE("stop io_pool.");
        for (auto& item : items_) {
            item->cycle->RunInCycle([=]() mutable {
                for (const auto& session : item->sessions) {
                    session.second->ForceClose();
                }
                item->sessions.clear();
                item->cycle->Exit();
            });
        }

        for (auto& item : items_) {
            item->thread->join();
        }
        is_running_ = false;
        thread_nbr_ = 0;
        items_.clear();
    }

    template <typename T>
    auto IOPool<T>::GetNextItem() -> Ptr<PoolItem<T>>
    {
        if (!is_running()) {
            return {};
        }

        auto index = last_++;
        last_ %= thread_nbr_;
        return items_[index];
    }

    template <typename T>
    auto IOPool<T>::GetItemByHash(std::size_t _hash_code) -> Ptr<PoolItem<T>>
    {
        return items_[_hash_code % thread_nbr_];
    }

} // namespace net
} // namespace hare

#endif // _HARE_NET_IO_POOL_H_