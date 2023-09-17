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

#ifndef _HARE_UTIL_QUEUE_H_
#define _HARE_UTIL_QUEUE_H_

#include <hare/base/util/non_copyable.h>

#include <condition_variable>
#include <mutex>
#include <vector>

namespace hare {

HARE_CLASS_API
template <typename T, typename Sequence = std::vector<T>>
class HARE_API CircularQueue : public NonCopyable {
 public:
  using SizeType = std::size_t;
  using ValueType = T;

 private:
  SizeType max_capacity_{};
  SizeType over_counter_{};
  SizeType head_{};
  SizeType tail_{};
  Sequence sequence_{};

 public:
  CircularQueue() = default;
  HARE_INLINE
  explicit CircularQueue(SizeType _max)
      : max_capacity_(_max), sequence_(_max) {}

  HARE_INLINE
  CircularQueue(CircularQueue&& _other) noexcept { Move(_other); }

  HARE_INLINE
  CircularQueue& operator=(CircularQueue&& _other) noexcept {
    Move(_other);
    return (*this);
  }

  void PushBack(T&& _item) {
    if (max_capacity_ > 0) {
      sequence_[tail_] = std::move(_item);
      tail_ = (tail_ + 1) % max_capacity_;

      if (tail_ == head_) {
        head_ = (head_ + 1) % max_capacity_;
        ++over_counter_;
      }
    }
  }

  HARE_INLINE auto Front() const -> const T& { return sequence_[head_]; }
  HARE_INLINE auto Front() -> T& { return sequence_[head_]; }

  HARE_INLINE
  auto Size() const -> SizeType {
    return tail_ >= head_ ? tail_ - head_ : max_capacity_ - head_ + tail_;
  }

  HARE_INLINE auto max_capacity() const -> SizeType { return max_capacity_; }

  HARE_INLINE
  auto At(SizeType i) const -> const T& {
    HARE_ASSERT(i < Size());
    return sequence_[(head_ + i) % max_capacity_];
  }

  HARE_INLINE
  auto PopFront() -> T {
    auto tmp = std::move(sequence_[head_]);
    head_ = (head_ + 1) % max_capacity_;
    return tmp;
  }

  HARE_INLINE
  void GetAll(Sequence& _seq) {
    auto i{0};
    while (!Empty()) {
      _seq[i] = std::move(PopFront());
    }
  }

  HARE_INLINE auto Empty() const -> bool { return tail_ == head_; }
  HARE_INLINE auto Full() const -> bool {
    return max_capacity_ > 0 ? ((tail_ + 1) % max_capacity_) == head_ : false;
  }

  HARE_INLINE auto over_counter() const -> SizeType { return over_counter_; }
  HARE_INLINE void ResetCounter() { over_counter_ = 0; }

 private:
  void Move(CircularQueue&& _other) noexcept {
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
class HARE_API BlockingQueue {
 public:
  using SizeType = typename CircularQueue<T>::SizeType;

 private:
  mutable std::mutex mutex_{};
  std::condition_variable pop_cv_{};
  std::condition_variable push_cv_{};
  CircularQueue<T> circular_;

 public:
  using ValueType = T;

  explicit BlockingQueue(SizeType _max_size) : circular_(_max_size) {}

  HARE_INLINE
  auto OverCounter() const -> SizeType {
    std::unique_lock<std::mutex> lock(mutex_);
    return circular_.over_counter();
  }

  HARE_INLINE
  auto Size() const -> SizeType {
    std::unique_lock<std::mutex> lock(mutex_);
    return circular_.Size();
  }

  HARE_INLINE
  void ResetCounter() {
    std::unique_lock<std::mutex> lock(mutex_);
    circular_.ResetCounter();
  }

  // try to enqueue and block if no room left
  void Enqueue(T& item) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      pop_cv_.wait(lock, [this] { return !this->circular_.Full(); });
      circular_.PushBack(std::move(item));
    }
    push_cv_.notify_one();
  }

  // enqueue immediately. overrun oldest message in the queue if no room left.
  void EnqueueNoWait(T& _item) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      circular_.PushBack(std::move(_item));
    }
    push_cv_.notify_one();
  }

  auto DequeueFor(T& _item, std::chrono::milliseconds _duration) -> bool {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (!push_cv_.wait_for(lock, _duration,
                             [this] { return !this->circular_.Empty(); })) {
        return false;
      }
      _item = std::move(circular_.PopFront());
    }
    pop_cv_.notify_one();
    return true;
  }

  void Dequeue(T& _item) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      push_cv_.wait(lock, [this] { return !this->circular_.Empty(); });
      _item = std::move(circular_.PopFront());
    }
    pop_cv_.notify_one();
  }

  auto Dequeues() -> std::vector<T> {
    std::vector<T> ret{};
    {
      std::unique_lock<std::mutex> lock(mutex_);
      push_cv_.wait(lock, [this] { return !this->circular_.Empty(); });
      ret.resize(circular_.Size());
      circular_.GetAll(ret);
    }
    pop_cv_.notify_one();

    return ret;
  }
};

}  // namespace hare

#endif  // _HARE_UTIL_QUEUE_H_