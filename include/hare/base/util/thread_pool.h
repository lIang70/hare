/**
 * @file hare/base/util/thread_pool.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with thrad_pool.h
 * @version 0.1-beta
 * @date 2023-02-09
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _HARE_BASE_THREAD_POOL_H_
#define _HARE_BASE_THREAD_POOL_H_

#include <hare/base/error.h>
#include <hare/base/util/non_copyable.h>
#include <hare/base/util/thread.h>

#include <vector>
#include <queue>

namespace hare {

class HARE_API ThreadPool : public NonCopyable {
    mutable std::mutex mutex_ {};
    std::condition_variable cv_for_not_empty_ {};
    std::condition_variable cv_for_not_full_ {};
    std::string name_ {};
    Thread::Task pool_init_callback_ {};
    std::vector<Thread::Ptr> threads_ {};
    std::deque<Thread::Task> queue_ {};
    size_t max_queue_size_ { 0 };
    bool running_ { false };

public:
    using Ptr = std::shared_ptr<ThreadPool>;

    explicit ThreadPool(std::string name = std::string("HTHREAD_POOL"));
    ~ThreadPool();

    inline auto name() const -> const std::string& { return name_; }

    // Must be called before start().
    auto setMaxQueueSize(size_t max_size) -> Error;
    auto setThreadInitCallback(const Thread::Task& init_cb) -> Error;

    inline auto isRunning() const -> bool { return running_; } 
    auto start(int32_t num_of_thread) -> Error;
    auto stop() -> Error;

    auto queueSize() const -> size_t;

    // Could block if maxQueueSize > 0
    // Call after stop() will return immediately.
    // There is no move-only version of std::function in C++ as of C++14.
    // So we don't need to overload a const& and an && versions
    // as we do in (Bounded)BlockingQueue.
    // https://stackoverflow.com/a/25408989
    auto run(Thread::Task task) -> Error;

private:
    auto isFull() const -> bool;

    auto take() -> Thread::Task;
    
    void loop();
};

} // namespace hare

#endif // _HARE_BASE_THREAD_POOL_H_
