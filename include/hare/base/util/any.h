/**
 * @file hare/base/util/any.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with any.h
 * @version 0.1-beta
 * @date 2023-07-08
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_UTIL_ANY_H_
#define _HARE_BASE_UTIL_ANY_H_

#include <hare/base/type_traits.h>

#include <utility>

namespace hare {
namespace util {

namespace detail {

template <typename Type>
struct FastTypeTag {
  constexpr static char dummy_var = 0;
};

using FastTypeIdType = const void*;

template <typename Type>
constexpr inline FastTypeIdType FastTypeId() {
  return &FastTypeTag<Type>::dummy_var;
}

HARE_NORETURN void ThrowBadAnyCast();

}  // namespace detail

class Any;

template <typename ValueType>
auto AnyCast(const Any& operand) -> ValueType;

template <typename ValueType>
auto AnyCast(Any& operand) -> ValueType;

template <typename ValueType>
auto AnyCast(Any&& operand) -> ValueType;

template <typename T>
auto AnyCast(const Any* operand) noexcept -> const T*;

template <typename T>
auto AnyCast(Any* operand) noexcept -> T*;

HARE_CLASS_API
class HARE_API Any {
  HARE_CLASS_API
  template <typename T>
  struct HARE_API IsInPlaceType : std::false_type {};

  HARE_CLASS_API
  struct HARE_API Base {
    virtual ~Base() = default;
    virtual auto Clone() const -> UPtr<Base> = 0;
    virtual auto ObjTypeId() const noexcept -> const void* = 0;
    virtual auto Type() const noexcept -> const std::type_info& = 0;
  };

  HARE_CLASS_API
  template <typename T>
  struct HARE_API Derived final : public Base {
    T value;

    template <typename... Args>
    explicit Derived(hare::detail::in_place_t /*tag*/, Args&&... args)
        : value(std::forward<Args>(args)...) {}

    auto Clone() const -> UPtr<Base> final {
      return UPtr<Base>(new Derived(hare::in_place, value));
    }

    auto ObjTypeId() const noexcept -> const void* final {
      return IdForType<T>();
    }

    auto Type() const noexcept -> const std::type_info& final {
      return typeid(T);
    }
  };

  UPtr<Base> ptr_{};

 public:
  constexpr Any() noexcept = default;

  Any(const Any& other)
      : ptr_(!other.HasValue() ? UPtr<Base>() : other.Clone()) {}

  Any(Any&& other) noexcept = default;

  template <
      typename T, typename VT = hare::decay_t<T>,
      hare::enable_if_t<!hare::disjunction<
          std::is_same<Any, VT>, IsInPlaceType<VT>,
          hare::negation<std::is_copy_constructible<VT>>>::value>* = nullptr>
  Any(T&& value) : ptr_(new Derived<VT>(in_place, std::forward<T>(value))) {}

  template <
      typename T, typename U, typename... Args, typename VT = hare::decay_t<T>,
      hare::enable_if_t<
          hare::conjunction<std::is_copy_constructible<VT>,
                            std::is_constructible<VT, std::initializer_list<U>&,
                                                  Args...>>::value>* = nullptr>
  explicit Any(hare::detail::in_place_type_t<T> /*tag*/,
               std::initializer_list<U> ilist, Args&&... args)
      : ptr_(new Derived<VT>(in_place, ilist, std::forward<Args>(args)...)) {}

  auto operator=(const Any& rhs) -> Any& {
    Any(rhs).Swap(*this);
    return *this;
  }

  auto operator=(Any&& rhs) noexcept -> Any& {
    Any(std::move(rhs)).Swap(*this);
    return *this;
  }

  template <typename T, typename VT = hare::decay_t<T>,
            hare::enable_if_t<hare::conjunction<
                hare::negation<std::is_same<VT, Any>>,
                std::is_copy_constructible<VT>>::value>* = nullptr>
  auto operator=(T&& rhs) -> Any& {
    Any tmp(hare::detail::in_place_type_t<VT>(), std::forward<T>(rhs));
    tmp.Swap(*this);
    return *this;
  }

  template <
      typename T, typename... Args, typename VT = hare::decay_t<T>,
      hare::enable_if_t<std::is_copy_constructible<VT>::value &&
                        std::is_constructible<VT, Args...>::value>* = nullptr>
  auto emplace(Args&&... args) HARE_ATTRIBUTE_LIFETIME_BOUND->VT& {
    Reset();  // NOTE: reset() is required here even in the world of exceptions.
    Derived<VT>* const object_ptr =
        new Derived<VT>(in_place, std::forward<Args>(args)...);
    ptr_ = UPtr<Base>(object_ptr);
    return object_ptr->value;
  }

  // Any::Reset()
  //
  // Resets the state of the `util::Any` object, destroying the contained object
  // if present.
  void Reset() noexcept { ptr_.reset(); }

  // Any::Swap()
  //
  // Swaps the passed value and the value of this `util::Any` object.
  void Swap(Any& other) noexcept { ptr_.swap(other.ptr_); }

  auto HasValue() const -> bool { return ptr_ != nullptr; }

  auto Type() const noexcept -> const std::type_info& {
    return HasValue() ? ptr_->Type() : typeid(void);
  }

 private:
  auto Clone() const -> UPtr<Base> {
    if (HasValue()) {
      return ptr_->Clone();
    }
    return nullptr;
  }

  template <typename T>
  static auto IdForType() -> const void* {
    // Note: This type dance is to make the behavior consistent with typeid.
    using NormalizedType =
        typename std::remove_cv<typename std::remove_reference<T>::type>::type;

    return detail::FastTypeId<NormalizedType>();
  }

  auto GetObjTypeId() const -> const void* {
    return ptr_ ? ptr_->ObjTypeId() : detail::FastTypeId<void>();
  }

  template <typename ValueType>
  friend auto AnyCast(const Any& operand) -> ValueType;

  template <typename ValueType>
  friend auto AnyCast(Any& operand) -> ValueType;

  template <typename ValueType>
  friend auto AnyCast(Any&& operand) -> ValueType;

  template <typename T>
  friend auto AnyCast(const Any* operand) noexcept -> const T*;

  template <typename T>
  friend auto AnyCast(Any* operand) noexcept -> T*;
};

template <typename ValueType>
auto AnyCast(const Any& operand) -> ValueType {
  using U = typename std::remove_cv<
      typename std::remove_reference<ValueType>::type>::type;
  static_assert(std::is_constructible<ValueType, const U&>::value,
                "Invalid ValueType");
  auto* const result = (AnyCast<U>)(&operand);
  if (result == nullptr) {
    detail::ThrowBadAnyCast();
  }
  return static_cast<ValueType>(*result);
}

template <typename ValueType>
auto AnyCast(Any& operand) -> ValueType {
  using U = typename std::remove_cv<
      typename std::remove_reference<ValueType>::type>::type;
  static_assert(std::is_constructible<ValueType, U&>::value,
                "Invalid ValueType");
  auto* result = (AnyCast<U>)(&operand);
  if (result == nullptr) {
    detail::ThrowBadAnyCast();
  }
  return static_cast<ValueType>(*result);
}

template <typename ValueType>
auto AnyCast(Any&& operand) -> ValueType {
  using U = typename std::remove_cv<
      typename std::remove_reference<ValueType>::type>::type;
  static_assert(std::is_constructible<ValueType, U>::value,
                "Invalid ValueType");
  return static_cast<ValueType>(std::move((AnyCast<U&>)(operand)));
}

template <typename T>
auto AnyCast(const Any* operand) noexcept -> const T* {
  using U =
      typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  return operand && operand->GetObjTypeId() == Any::IdForType<U>()
             ? std::addressof(
                   static_cast<const Any::Derived<U>*>(operand->ptr_.get())
                       ->value)
             : nullptr;
}

template <typename T>
auto AnyCast(Any* operand) noexcept -> T* {
  using U =
      typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  return operand && operand->GetObjTypeId() == Any::IdForType<U>()
             ? std::addressof(
                   static_cast<Any::Derived<U>*>(operand->ptr_.get())->value)
             : nullptr;
}

}  // namespace util
}  // namespace hare

#endif  // _HARE_BASE_UTIL_ANY_H_
