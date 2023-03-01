#include <hare/base/exception.h>
#include <hare/base/logging.h>
#include <hare/base/thread_pool.h>

#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

namespace hare {

class ThreadPool::Data {
public:
    mutable std::mutex mutex_ {};
    std::condition_variable cv_for_not_empty_ {};
    std::condition_variable cv_for_not_full_ {};
    std::string name_ {};
    Thread::Task pool_init_callback_ {};
    std::vector<std::unique_ptr<Thread>> threads_ {};
    std::deque<Thread::Task> queue_ {};
    size_t max_queue_size_ { 0 };
    bool running_ { false };

    explicit Data(std::string name)
        : name_(std::move(name))
    {
    }
};

ThreadPool::ThreadPool(const std::string& name)
    : d_(new Data(name))
{
}

ThreadPool::~ThreadPool()
{
    if (d_->running_) {
        stop();
    }

    delete d_;
}

const std::string& ThreadPool::name() const
{
    return d_->name_;
}

// Must be called before start().
void ThreadPool::setMaxQueueSize(size_t max_size)
{
    d_->max_queue_size_ = max_size;
}

void ThreadPool::setThreadInitCallback(const Thread::Task& cb)
{
    d_->pool_init_callback_ = cb;
}

void ThreadPool::start(int num_of_thread)
{
    HARE_ASSERT(d_->threads_.empty(), "Thread pool is not empty.");
    d_->running_ = true;
    d_->threads_.clear();
    for (auto i = 0; i < num_of_thread; ++i) {
        d_->threads_.emplace_back(new Thread(std::bind(&ThreadPool::loop, this), d_->name_ + std::to_string(i)));
        d_->threads_[i]->start();
    }
    if (num_of_thread == 0 && d_->pool_init_callback_) {
        d_->pool_init_callback_();
    }
}

void ThreadPool::stop()
{
    {
        std::lock_guard<std::mutex> lock(d_->mutex_);
        d_->running_ = false;
        d_->cv_for_not_empty_.notify_all();
        d_->cv_for_not_full_.notify_all();
    }
    for (auto& thr : d_->threads_) {
        thr->join();
    }
}

size_t ThreadPool::queueSize() const
{
    std::lock_guard<std::mutex> lock(d_->mutex_);
    return d_->queue_.size();
}

void ThreadPool::run(Thread::Task task)
{
    if (d_->threads_.empty()) {
        task();
    } else {
        std::unique_lock<std::mutex> lock(d_->mutex_);
        while (d_->max_queue_size_ > 0 && d_->queue_.size() >= d_->max_queue_size_ && d_->running_) {
            d_->cv_for_not_full_.wait(lock);
        }
        if (!d_->running_)
            return;
        HARE_ASSERT(d_->max_queue_size_ == 0 || d_->queue_.size() < d_->max_queue_size_, "Thread pool is full or the max size of queue is zero.");

        d_->queue_.push_back(std::move(task));
        d_->cv_for_not_empty_.notify_one();
    }
}

Thread::Task ThreadPool::take()
{
    std::unique_lock<std::mutex> lock(d_->mutex_);
    // always use a while-loop, due to spurious wakeup
    while (d_->queue_.empty() && d_->running_) {
        d_->cv_for_not_empty_.wait(lock);
    }
    Thread::Task task {};
    if (!d_->queue_.empty()) {
        task = std::move(d_->queue_.front());
        d_->queue_.pop_front();
        if (d_->max_queue_size_ > 0) {
            d_->cv_for_not_full_.notify_one();
        }
    }
    return task;
}

bool ThreadPool::isFull() const
{
    std::unique_lock<std::mutex> lock(d_->mutex_);
    return d_->max_queue_size_ > 0 && d_->queue_.size() >= d_->max_queue_size_;
}

void ThreadPool::loop()
{
    try {
        if (d_->pool_init_callback_) {
            d_->pool_init_callback_();
        }
        while (d_->running_) {
            auto task(take());
            if (task) {
                task();
            }
        }
    } catch (const Exception& ex) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", d_->name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        std::abort();
    } catch (const std::exception& ex) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", d_->name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        std::abort();
    } catch (...) {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", d_->name_.c_str());
        throw; // rethrow
    }
}

}
