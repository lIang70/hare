/**
 * @file hare/base/type_traits.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with type_traits.h
 * @version 0.1-beta
 * @date 2023-08-22
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_TYPE_TRAITS_H_
#define _HARE_BASE_TYPE_TRAITS_H_

#include <hare/base/fwd.h>

#include <type_traits>

namespace hare {
namespace detail {

template <typename... Ts>
struct VoidImpl {
  using type = void;
};

}  // namespace detail

// void_t()
//
// Ignores the type of any its arguments and returns `void`. In general, this
// metafunction allows you to create a general case that maps to `void` while
// allowing specializations that map to specific types.
template <typename... Ts>
using void_t = typename detail::VoidImpl<Ts...>::type;

// conjunction
//
// Performs a compile-time logical AND operation on the passed types (which
// must have  `::value` members convertible to `bool`. Short-circuits if it
// encounters any `false` members (and does not compare the `::value` members
// of any remaining arguments).
template <typename... Ts>
struct conjunction : std::true_type {};

template <typename T, typename... Ts>
struct conjunction<T, Ts...>
    : std::conditional<T::value, conjunction<Ts...>, T>::type {};

template <typename T>
struct conjunction<T> : T {};

// disjunction
//
// Performs a compile-time logical OR operation on the passed types (which
// must have  `::value` members convertible to `bool`. Short-circuits if it
// encounters any `true` members (and does not compare the `::value` members
// of any remaining arguments).
template <typename... Ts>
struct disjunction : std::false_type {};

template <typename T, typename... Ts>
struct disjunction<T, Ts...>
    : std::conditional<T::value, T, disjunction<Ts...>>::type {};

template <typename T>
struct disjunction<T> : T {};

// negation
//
// Performs a compile-time logical NOT operation on the passed type (which
// must have  `::value` members convertible to `bool`.
template <typename T>
struct negation : std::integral_constant<bool, !T::value> {};

// is_copy_assignable()
// is_move_assignable()
// is_trivially_destructible()
// is_trivially_default_constructible()
// is_trivially_move_constructible()
// is_trivially_copy_constructible()
// is_trivially_move_assignable()
// is_trivially_copy_assignable()
//
// Historical note: Abseil once provided implementations of these type traits
// for platforms that lacked full support. New code should prefer to use the
// std variants.
//
// See the documentation for the STL <type_traits> header for more information:
// https://en.cppreference.com/w/cpp/header/type_traits
using std::is_copy_assignable;
using std::is_move_assignable;
using std::is_trivially_copy_assignable;
using std::is_trivially_copy_constructible;
using std::is_trivially_default_constructible;
using std::is_trivially_destructible;
using std::is_trivially_move_assignable;
using std::is_trivially_move_constructible;

// remove_cvref()
//
// C++11 compatible implementation of std::remove_cvref which was added in
// C++20.
template <typename T>
struct remove_cvref {
  using type =
      typename std::remove_cv<typename std::remove_reference<T>::type>::type;
};

template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

// -----------------------------------------------------------------------------
// C++14 "_t" trait aliases
// -----------------------------------------------------------------------------

template <typename T>
using remove_cv_t = typename std::remove_cv<T>::type;

template <typename T>
using remove_const_t = typename std::remove_const<T>::type;

template <typename T>
using remove_volatile_t = typename std::remove_volatile<T>::type;

template <typename T>
using add_cv_t = typename std::add_cv<T>::type;

template <typename T>
using add_const_t = typename std::add_const<T>::type;

template <typename T>
using add_volatile_t = typename std::add_volatile<T>::type;

template <typename T>
using remove_reference_t = typename std::remove_reference<T>::type;

template <typename T>
using add_lvalue_reference_t = typename std::add_lvalue_reference<T>::type;

template <typename T>
using add_rvalue_reference_t = typename std::add_rvalue_reference<T>::type;

template <typename T>
using remove_pointer_t = typename std::remove_pointer<T>::type;

template <typename T>
using add_pointer_t = typename std::add_pointer<T>::type;

template <typename T>
using make_signed_t = typename std::make_signed<T>::type;

template <typename T>
using make_unsigned_t = typename std::make_unsigned<T>::type;

template <typename T>
using remove_extent_t = typename std::remove_extent<T>::type;

template <typename T>
using remove_all_extents_t = typename std::remove_all_extents<T>::type;

template <typename T>
using decay_t = typename std::decay<T>::type;

template <bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template <bool B, typename T, typename F>
using conditional_t = typename std::conditional<B, T, F>::type;

template <typename... T>
using common_type_t = typename std::common_type<T...>::type;

template <typename T>
using underlying_type_t = typename std::underlying_type<T>::type;

}  // namespace hare

#endif  // _HARE_BASE_TYPE_TRAITS_H_