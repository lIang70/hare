/**
 * @file hare/base/thread/thread_pool.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with thread_pool.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_THREAD_POOL_H_
#define _HARE_BASE_THREAD_POOL_H_

#include <hare/base/thread/thread.h>

#include <condition_variable>
#include <queue>
#include <vector>

namespace hare {

class HARE_API thread_pool : public non_copyable {
    mutable std::mutex mutex_ {};
    std::condition_variable cv_for_not_empty_ {};
    std::condition_variable cv_for_not_full_ {};
    std::string name_ {};
    thread::task pool_init_callback_ {};
    std::vector<thread::ptr> threads_ {};
    std::deque<thread::task> queue_ {};
    size_t max_queue_size_ { 0 };
    bool running_ { false };

public:
    using ptr = ptr<thread_pool>;

    explicit thread_pool(std::string name = std::string("HTHREAD_POOL"));
    ~thread_pool();

    inline auto name() const -> const std::string& { return name_; }
    inline auto running() const -> bool { return running_; }

    // Must be called before start().
    inline void set_max_queue(size_t _max_size) { max_queue_size_ = _max_size; }
    inline void set_thread_init_callback(const thread::task& _init_cb) { pool_init_callback_ = _init_cb; }

    void start(int32_t _num_of_thread);
    void stop();

    auto queue_size() const -> size_t;

    /**
     * Could block if max_queue_size_ > 0
     *  Call after stop() will return immediately.
     *  There is no move-only version of std::function in C++ as of C++14.
     *  So we don't need to overload a const& and an && versions
     *  as we do in (Bounded)BlockingQueue.
     *  https://stackoverflow.com/a/25408989
     **/
    void run(thread::task task);

private:
    auto full() const -> bool;
    auto take() -> thread::task;

    void loop();
};

} // namespace hare

#endif // !_HARE_BASE_THREAD_POOL_H_