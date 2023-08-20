/**
 * @file hare/base/util/thread_pool.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with thread_pool.h
 * @version 0.1-beta
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_UTIL_THREAD_POOL_H_
#define _HARE_LOG_UTIL_THREAD_POOL_H_

#include <hare/base/exception.h>
#include <hare/base/util/queue.h>

#include <thread>

namespace hare {
namespace util {

    enum class Policy {
        BLOCK_RETRY, // Block / yield / sleep until message can be enqueued
        DISCARD // Discard the message it enqueue fails
    };

    HARE_CLASS_API
    template <typename T>
    class HARE_API ThreadPool {
    public:
        using ValueType = T;
        using TaskHandle = std::function<bool(ValueType&)>;

    private:
        BlockingQueue<T> queue_ {};
        std::vector<std::thread> threads_ {};
        TaskHandle handle_ {};

    public:
        HARE_INLINE 
        ThreadPool(std::size_t _max_items, std::size_t _thr_n, TaskHandle _thr_task)
            : queue_(_max_items)
            , threads_(_thr_n)
            , handle_(std::move(_thr_task))
        {
            if (_thr_n == 0 || _thr_n >= 1024) {
                throw Exception("invalid _thr_n param (valid range 1-1024)");
            }
        }

        HARE_INLINE
        ~ThreadPool()
        {
            Join();
        }

        HARE_INLINE
        void Start(const Task& _before_thr, const Task& _after_thr)
        {
            for (auto& thr : threads_) {
                thr = std::thread([this, _before_thr, _after_thr] {
                    _before_thr();
                    this->Loop();
                    _after_thr();
                });
            }
        }

        HARE_INLINE
        void Join()
        {
            for (auto& thr : threads_) {
                if (thr.joinable()) {
                    thr.join();
                }
            }
        }

        HARE_INLINE
        auto ThreadSize() const -> std::size_t
        {
            return threads_.size();
        }

        HARE_INLINE
        auto OverCounter() const -> std::size_t
        {
            return queue_.OverCounter();
        }

        HARE_INLINE
        void ResetCounter()
        {
            queue_.ResetCounter();
        }

        HARE_INLINE
        auto QueueSize() -> std::size_t
        {
            return queue_.Size();
        }

        HARE_INLINE
        void Post(T&& _item, Policy _policy)
        {
            switch (_policy) {
            case Policy::BLOCK_RETRY:
                queue_.Enqueue(_item);
                break;
            case Policy::DISCARD:
                queue_.EnqueueNoWait(_item);
                break;
            default:
                assert(false);
                break;
            }
        }

    private:
        HARE_INLINE
        void Loop()
        {
            for (;;) {
                T item {};
                queue_.Dequeue(item);

                auto ret = handle_(item);
                if (!ret) {
                    break;
                }
            }
        }
    };

} // namespace util
} // namespace hare

#endif // _HARE_LOG_UTIL_THREAD_POOL_H_