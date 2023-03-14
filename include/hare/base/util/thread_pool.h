#ifndef _HARE_BASE_THREAD_POOL_H_
#define _HARE_BASE_THREAD_POOL_H_

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

    explicit ThreadPool(std::string name = std::string("THREAD_POOL"));
    ~ThreadPool();

    inline const std::string& name() const { return name_; }

    // Must be called before start().
    inline void setMaxQueueSize(size_t max_size) { max_queue_size_ = max_size; }
    void setThreadInitCallback(const Thread::Task& cb);

    inline bool isRunning() const { return running_; } 
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
