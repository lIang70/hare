/**
 * @file hare/base/util/queue.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with queue.h
 * @version 0.1-beta
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_UTIL_QUEUE_H_
#define _HARE_LOG_UTIL_QUEUE_H_

#include <hare/base/util/non_copyable.h>

#include <condition_variable>
#include <mutex>
#include <vector>

namespace hare {
namespace util {

    HARE_CLASS_API
    template <typename T,
        typename _Sequence = std::vector<T>>
    class HARE_API circular_queue : public non_copyable {
    public:
        using size_type = std::size_t;
        using value_type = T;

    private:
        size_type max_capacity_ {};
        size_type over_counter_ {};
        size_type head_ {};
        size_type tail_ {};
        _Sequence sequence_ {};

    public:
        circular_queue() = default;
        HARE_INLINE
        explicit circular_queue(size_type _max)
            : max_capacity_(_max)
            , sequence_(_max)
        { }

        HARE_INLINE
        circular_queue(circular_queue&& _other) noexcept
        { move(_other); }

        HARE_INLINE
        circular_queue& operator=(circular_queue&& _other) noexcept
        { move(_other); return (*this); }

        void push_back(T&& _item)
        {
            if (max_capacity_ > 0) {
                sequence_[tail_] = std::move(_item);
                tail_ = (tail_ + 1) % max_capacity_;

                if (tail_ == head_) {
                    head_ = (head_ + 1) % max_capacity_;
                    ++over_counter_;
                }
            }
        }

        HARE_INLINE auto front() const -> const T& { return sequence_[head_]; }
        HARE_INLINE auto front() -> T& { return sequence_[head_]; }

        HARE_INLINE
        auto size() const -> size_type
        { return tail_ >= head_ ? tail_ - head_ : max_capacity_ - head_ + tail_; }

        HARE_INLINE auto capacity() const -> size_type { return max_capacity_; }

        HARE_INLINE
        auto at(size_type i) const -> const T&
        {
            assert(i < size());
            return sequence_[(head_ + i) % max_capacity_];
        }

        HARE_INLINE
        auto pop_front() -> T
        {
            auto tmp = std::move(sequence_[head_]);
            head_ = (head_ + 1) % max_capacity_;
            return tmp;
        }

        HARE_INLINE
        void get_all(_Sequence& _seq)
        {
            auto i { 0 };
            while (!empty()) {
                _seq[i] = std::move(pop_front());
            }
        }

        HARE_INLINE auto empty() const -> bool { return tail_ == head_; }
        HARE_INLINE auto full() const -> bool { return max_capacity_ > 0 ? ((tail_ + 1) % max_capacity_) == head_ : false; }

        HARE_INLINE auto over_counter() const -> size_type { return over_counter_; }
        HARE_INLINE void reset_counter() { over_counter_ = 0; }

    private:
        void move(circular_queue&& _other) noexcept
        {
            max_capacity_ = _other.max_capacity_;
            head_ = _other.head_;
            tail_ = _other.tail_;
            over_counter_ = _other.over_counter_;
            sequence_ = std::move(_other.sequence_);

            // put &&other in disabled, but valid state
            _other.max_capacity_ = 0;
            _other.head_ = _other.tail_ = 0;
            _other.over_counter_ = 0;
        }

    };

    HARE_CLASS_API
    template <typename T>
    class HARE_API blocking_queue {
    public:
        using size_type = typename circular_queue<T>::size_type;

    private:
        mutable std::mutex mutex_ {};
        std::condition_variable pop_cv_ {};
        std::condition_variable push_cv_ {};
        circular_queue<T> circular_;

    public:
        using value_type = T;

        explicit blocking_queue(size_type _max_size)
            : circular_(_max_size)
        {
        }

        HARE_INLINE
        auto over_counter() const -> size_type
        {
            std::unique_lock<std::mutex> lock(mutex_);
            return circular_.over_counter();
        }

        HARE_INLINE
        auto size() const -> size_type
        {
            std::unique_lock<std::mutex> lock(mutex_);
            return circular_.size();
        }

        HARE_INLINE
        void reset_counter()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            circular_.reset_counter();
        }

        // try to enqueue and block if no room left
        void enqueue(T& item)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                pop_cv_.wait(lock, [this] { return !this->circular_.full(); });
                circular_.push_back(std::move(item));
            }
            push_cv_.notify_one();
        }

        // enqueue immediately. overrun oldest message in the queue if no room left.
        void enqueue_nowait(T& _item)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                circular_.push_back(std::move(_item));
            }
            push_cv_.notify_one();
        }

        auto dequeue_for(T& _item, std::chrono::milliseconds _duration) -> bool
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (!push_cv_.wait_for(lock, _duration, [this] { return !this->circular_.empty(); })) {
                    return false;
                }
                _item = std::move(circular_.pop_front());
            }
            pop_cv_.notify_one();
            return true;
        }

        void dequeue(T& _item)
        {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                push_cv_.wait(lock, [this] { return !this->circular_.empty(); });
                _item = std::move(circular_.pop_front());
            }
            pop_cv_.notify_one();
        }

        auto dequeues() -> std::vector<T>
        {
            std::vector<T> ret {};
            {
                std::unique_lock<std::mutex> lock(mutex_);
                push_cv_.wait(lock, [this] { return !this->circular_.empty(); });
                ret.resize(circular_.size());
                circular_.get_all(ret);
            }
            pop_cv_.notify_one();

            return ret;
        }
    };

} // namespace util
} // namespace hare

#endif // _HARE_LOG_UTIL_QUEUE_H_