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

    enum class POLICY {
        BLOCK_RETRY, // Block / yield / sleep until message can be enqueued
        DISCARD // Discard the message it enqueue fails
    };

    HARE_CLASS_API
    template <typename T>
    class HARE_API thread_pool {
    public:
        using value_type = T;
        using task_handle = std::function<bool(T&)>;

    private:
        blocking_queue<T> queue_ {};
        std::vector<std::thread> threads_ {};
        task_handle handle_ {};

    public:
        HARE_INLINE thread_pool(std::size_t _max_items, std::size_t _thr_n, task_handle _thr_task)
            : queue_(_max_items)
            , threads_(_thr_n)
            , handle_(std::move(_thr_task))
        {
            if (_thr_n == 0 || _thr_n >= 1024) {
                throw exception("invalid _thr_n param (valid range 1-1024)");
            }
        }

        ~thread_pool()
        {
            join();
        }

        HARE_INLINE
        void start(const task& _before_thr, const task& _after_thr)
        {
            for (auto& thr : threads_) {
                thr = std::move(std::thread([this, _before_thr, _after_thr] {
                    _before_thr();
                    this->loop();
                    _after_thr();
                }));
            }
        }

        HARE_INLINE
        void join()
        {
            for (auto& thr : threads_) {
                if (thr.joinable()) {
                    thr.join();
                }
            }
        }

        HARE_INLINE
        auto thr_num() const -> std::size_t
        {
            return threads_.size();
        }

        HARE_INLINE
        auto over_counter() const -> std::size_t
        {
            return queue_.over_counter();
        }

        HARE_INLINE
        void reset_counter()
        {
            queue_.reset_counter();
        }

        HARE_INLINE
        auto queue_size() -> std::size_t
        {
            return queue_.size();
        }

        HARE_INLINE
        void post(T&& _item, POLICY _policy)
        {
            switch (_policy) {
            case POLICY::BLOCK_RETRY:
                queue_.enqueue(_item);
                break;
            case POLICY::DISCARD:
                queue_.enqueue_nowait(_item);
                break;
            default:
                assert(false);
                break;
            }
        }

    private:
        void loop()
        {
            for (;;) {
                T item {};
                queue_.dequeue(item);

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