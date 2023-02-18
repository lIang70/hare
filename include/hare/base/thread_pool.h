#ifndef _HARE_BASE_THREAD_POOL_H_
#define _HARE_BASE_THREAD_POOL_H_

#include <hare/base/detail/non_copyable.h>
#include <hare/base/thread.h>

namespace hare {

class HARE_API ThreadPool : public NonCopyable {
    class Data;
    Data* d_ { nullptr };

public:
    explicit ThreadPool(const std::string& name = std::string("ThreadPool"));
    ~ThreadPool();

    const std::string& name() const;

    // Must be called before start().
    void setMaxQueueSize(size_t max_size);
    void setThreadInitCallback(const Thread::Task& cb);

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
