#include <hare/base/thread_pool.h>
#include <hare/base/exception.h>
#include <hare/base/logging.h>

#include <cstdio>

namespace hare {

ThreadPool::ThreadPool(const std::string& name)
    : name_(name)
{
}

ThreadPool::~ThreadPool()
{
    if (running_) {
        stop();
    }
}

void ThreadPool::start(int num_of_thread)
{
    HARE_ASSERT(threads_.empty(), "Thread pool is not empty.");
    running_ = true;
    threads_.reserve(num_of_thread);
    for (int i = 0; i < num_of_thread; ++i) {
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::loop, this), name_ + std::to_string(i)));
        threads_[i]->start();
    }
    if (num_of_thread == 0 && pool_init_callback_) {
        pool_init_callback_();
    }
}

void ThreadPool::stop()
{
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        running_ = false;
        cv_for_not_empty_.notify_all();
        cv_for_not_full_.notify_all();
    }
    for (auto& thr : threads_) {
        thr->join();
    }
}

size_t ThreadPool::queueSize() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return queue_.size();
}

void ThreadPool::run(Thread::Task task)
{
    if (threads_.empty()) {
        task();
    } else {
        std::unique_lock<std::recursive_mutex> lock(mutex_);
        while (isFull() && running_) {
            cv_for_not_full_.wait(lock);
        }
        if (!running_)
            return;
        HARE_ASSERT(!isFull(), "Thread pool is full.");

        queue_.push_back(std::move(task));
        cv_for_not_empty_.notify_one();
    }
}

Thread::Task ThreadPool::take()
{
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    while (queue_.empty() && running_) {
        cv_for_not_empty_.wait(lock);
    }
    Thread::Task task;
    if (!queue_.empty()) {
        task = queue_.front();
        queue_.pop_front();
        if (max_queue_size_ > 0) {
            cv_for_not_full_.notify_one();
        }
    }
    return task;
}

bool ThreadPool::isFull() const
{
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    return max_queue_size_ > 0 && queue_.size() >= max_queue_size_;
}

void ThreadPool::loop()
{
    try {
        if (pool_init_callback_) {
            pool_init_callback_();
        }
        while (running_) {
            Thread::Task task(take());
            if (task) {
                task();
            }
        }
    } catch (const Exception& ex) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        std::abort();
    } catch (const std::exception& ex) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        std::abort();
    } catch (...) {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
}

}
