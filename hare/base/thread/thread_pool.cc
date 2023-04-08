#include "hare/base/error.h"
#include <hare/base/util/thread_pool.h>
#include <hare/base/exception.h>
#include <hare/base/logging.h>

namespace hare {

ThreadPool::ThreadPool(std::string name)
    : name_(std::move(name))
{
}

ThreadPool::~ThreadPool()
{
    if (running_) {
        stop();
    }
}

auto ThreadPool::setMaxQueueSize(size_t max_size) -> Error
{
    if (isRunning()) {
        return Error(HARE_ERROR_TPOOL_ALREADY_RUNNING);
    }
    max_queue_size_ = max_size;
    return Error(HARE_ERROR_SUCCESS);
}

auto ThreadPool::setThreadInitCallback(const Thread::Task& init_cb) -> Error
{
    if (isRunning()) {
        return Error(HARE_ERROR_TPOOL_ALREADY_RUNNING);
    }
    pool_init_callback_ = init_cb;
    return Error(HARE_ERROR_SUCCESS);
}

auto ThreadPool::start(int num_of_thread) -> Error
{
    if (!isRunning()) {
        return Error(HARE_ERROR_TPOOL_ALREADY_RUNNING);
    }

    HARE_ASSERT(threads_.empty(), "Thread pool is not empty.");

    running_ = true;
    threads_.clear();
    for (auto i = 0; i < num_of_thread; ++i) {
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::loop, this), name_ + std::to_string(i)));
        auto error = threads_[i]->start();
        if (!error) {
            running_ = false;
            threads_.clear();
            return error;
        }
    }

    if (num_of_thread == 0 && pool_init_callback_) {
        pool_init_callback_();
        return Error(HARE_ERROR_TPOOL_THREAD_ZERO);
    }

    return Error(HARE_ERROR_SUCCESS);
}

auto ThreadPool::stop() -> Error
{
    if (!isRunning()) {
        return Error(HARE_ERROR_TPOOL_NOT_RUNNING);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
        cv_for_not_empty_.notify_all();
        cv_for_not_full_.notify_all();
    }

    for (auto& thr : threads_) {
        thr->join();
    }

    return Error(HARE_ERROR_SUCCESS);
}

auto ThreadPool::queueSize() const -> size_t
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

auto ThreadPool::run(Thread::Task task) -> Error
{
    if (threads_.empty()) {
        SYS_ERROR() << "Cannot run task in the thread pool[" << this << "] because the number of threads is zero.";
        task();
        return Error(HARE_ERROR_TPOOL_THREAD_ZERO);
    }         
    
    std::unique_lock<std::mutex> lock(mutex_);
    while (max_queue_size_ > 0 && queue_.size() >= max_queue_size_ && running_) {
        cv_for_not_full_.wait(lock);
    }

    if (!running_) {
        return Error(HARE_ERROR_TPOOL_NOT_RUNNING);
    }

    HARE_ASSERT(max_queue_size_ == 0 || queue_.size() < max_queue_size_, "Thread pool is full or the max size of queue is zero.");

    queue_.push_back(std::move(task));
    cv_for_not_empty_.notify_one();

    return Error(HARE_ERROR_SUCCESS);
}

auto ThreadPool::isFull() const -> bool
{
    std::unique_lock<std::mutex> lock(mutex_);
    return max_queue_size_ > 0 && queue_.size() >= max_queue_size_;
}

auto ThreadPool::take() -> Thread::Task
{
    std::unique_lock<std::mutex> lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    while (queue_.empty() && running_) {
        cv_for_not_empty_.wait(lock);
    }
    Thread::Task task {};
    if (!queue_.empty()) {
        task = std::move(queue_.front());
        queue_.pop_front();
        if (max_queue_size_ > 0) {
            cv_for_not_full_.notify_one();
        }
    }
    return task;
}

void ThreadPool::loop()
{
    if (pool_init_callback_) {
        pool_init_callback_();
    }
    while (running_) {
        auto task(take());
        if (task) {
            task();
        }
    }
}

} // namespace hare
