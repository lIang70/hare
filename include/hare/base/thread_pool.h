#ifndef _HARE_BASE_THREAD_POOL_H_
#define _HARE_BASE_THREAD_POOL_H_

#include <hare/base/detail/non_copyable.h>
#include <hare/base/thread.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

namespace hare {

class ThreadPool : public NonCopyable {
    mutable std::recursive_mutex mutex_ {};
    std::condition_variable_any cv_for_not_empty_ {};
    std::condition_variable_any cv_for_not_full_ {};
    std::string name_ {};
    Thread::Task pool_init_callback_ {};
    std::vector<std::unique_ptr<Thread>> threads_ {};
    std::deque<Thread::Task> queue_ {};
    size_t max_queue_size_ { 0 };
    bool running_ { false };

public:
    explicit ThreadPool(const std::string& name = std::string("ThreadPool"));
    ~ThreadPool();

    // Must be called before start().
    inline void setMaxQueueSize(size_t max_size) { max_queue_size_ = max_size; }
    inline void setThreadInitCallback(const Thread::Task& cb) { pool_init_callback_ = cb; }
    inline const std::string& name() const { return name_; }

    void start(int32_t num_of_thread);
    void stop();

    size_t queueSize() const;

    // Could block if maxQueueSize > 0
    // Call after stop() will return immediately.
    // There is no move-only version of std::function in C++ as of C++14.
    // So we don't need to overload a const& and an && versions
    // as we do in (Bounded)BlockingQueue.
    // https://stackoverflow.com/a/25408989
    void run(Thread::Task f);

private:
    bool isFull() const;
    Thread::Task take();

    void loop();
};

}

#endif // _HARE_BASE_THREAD_POOL_H_
