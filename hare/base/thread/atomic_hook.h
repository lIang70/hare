/**
 * @file hare/base/thread/atomic_hook.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Taken from [abseil-cpp](https://github.com/abseil/abseil-cpp).
 * @version 0.2-beta
 * @date 2023-08-23
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_THREAD_ATOMIC_HOOK_H_
#define _HARE_BASE_THREAD_ATOMIC_HOOK_H_

#include <hare/base/fwd.h>

#include <atomic>

namespace hare {

template <typename T>
class AtomicHook;

template <typename ReturnType, typename... Args>
HARE_CLASS_API class HARE_API AtomicHook<ReturnType (*)(Args...)> {
 public:
  using FnPtr = ReturnType (*)(Args...);

  // Constructs an object that by default performs a no-op (and
  // returns a default constructed object) when no hook as been registered.
  constexpr AtomicHook() : AtomicHook(DummyFunction) {}

  // Constructs an object that by default dispatches to/returns the
  // pre-registered default_fn when no hook has been registered at runtime.
  explicit constexpr AtomicHook(FnPtr default_fn)
      : hook_(default_fn), default_fn_(default_fn) {}

  void Store(FnPtr fn) {
    bool success = DoStore(fn);
    static_cast<void>(success);
    assert(success);
  }

  // Invokes the registered callback.  If no callback has yet been registered, a
  // default-constructed object of the appropriate type is returned instead.
  template <typename... CallArgs>
  auto operator()(CallArgs&&... args) const -> ReturnType {
    return DoLoad()(std::forward<CallArgs>(args)...);
  }

  // Returns the registered callback, or nullptr if none has been registered.
  // Useful if client code needs to conditionalize behavior based on whether a
  // callback was registered.
  //
  // Note that AtomicHook.Load()() and AtomicHook() have different semantics:
  // operator()() will perform a no-op if no callback was registered, while
  // Load()() will dereference a null function pointer.  Prefer operator()() to
  // Load()() unless you must conditionalize behavior on whether a hook was
  // registered.
  auto Load() const -> FnPtr {
    FnPtr ptr = DoLoad();
    return (ptr == DummyFunction) ? nullptr : ptr;
  }

 private:
  static auto DummyFunction(Args...) -> ReturnType { return ReturnType(); }

  // Current versions of MSVC (as of September 2017) have a broken
  // implementation of std::atomic<T*>:  Its constructor attempts to do the
  // equivalent of a reinterpret_cast in a constexpr context, which is not
  // allowed.
  //
  // This causes an issue when building with LLVM under Windows.  To avoid this,
  // we use a less-efficient, intptr_t-based implementation on Windows.
#if !defined(H_OS_WIN)
  // Return the stored value, or DummyFunction if no value has been stored.
  auto DoLoad() const -> FnPtr { return hook_.load(std::memory_order_acquire); }

  // Store the given value.  Returns false if a different value was already
  // stored to this object.
  auto DoStore(FnPtr fn) -> bool {
    assert(fn);
    FnPtr expected = default_fn_;
    const bool store_succeeded = hook_.compare_exchange_strong(
        expected, fn, std::memory_order_acq_rel, std::memory_order_acquire);
    const bool same_value_already_stored = (expected == fn);
    return store_succeeded || same_value_already_stored;
  }

  std::atomic<FnPtr> hook_{};
#else
  // Use a sentinel value unlikely to be the address of an actual function.
  static constexpr intptr_t kUninitialized = 0;

  static_assert(sizeof(intptr_t) >= sizeof(FnPtr),
                "intptr_t can't contain a function pointer");

  auto DoLoad() const -> FnPtr {
    const intptr_t value = hook_.load(std::memory_order_acquire);
    if (value == kUninitialized) {
      return default_fn_;
    }
    return reinterpret_cast<FnPtr>(value);
  }

  auto DoStore(FnPtr fn) -> bool {
    HARE_ASSERT(fn);
    const auto value = reinterpret_cast<intptr_t>(fn);
    intptr_t expected = kUninitialized;
    const bool store_succeeded = hook_.compare_exchange_strong(
        expected, value, std::memory_order_acq_rel, std::memory_order_acquire);
    const bool same_value_already_stored = (expected == value);
    return store_succeeded || same_value_already_stored;
  }

  std::atomic<intptr_t> hook_{};
#endif

  const FnPtr default_fn_;
};

}  // namespace hare

#endif  // _HARE_BASE_THREAD_ATOMIC_HOOK_H_