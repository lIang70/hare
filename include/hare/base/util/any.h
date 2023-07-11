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

#include <hare/base/fwd.h>

#include <forward_list>
#include <type_traits>
#include <typeindex>

namespace hare {
namespace util {

    namespace detail {

        struct base {
            using uptr = uptr<base>;
            virtual ~base() = default;
            virtual auto clone() const -> uptr = 0;
        };

        template <typename T>
        struct derived : public base {
            T value_;

            template <typename... Args>
            explicit derived(Args&&... args)
                : value_(std::forward<Args>(args)...)
            {
            }

            auto clone() const -> uptr override
            {
                return uptr(new derived(value_));
            }
        };

    } // namespace detail

    class any {
        detail::base::uptr ptr_ {};
        std::type_index type_index_;

    public:
        any();
        any(const any& other);
        any(any&& other) noexcept;

        auto operator=(const any& a) -> any&;

        HARE_INLINE
        auto is_null() const -> bool { return !ptr_; }

        template <class T, class = typename std::enable_if<!std::is_same<typename std::decay<T>::type, any>::value, T>::type>
        explicit any(T&& t)
            : ptr_(new detail::derived<typename std::decay<T>::type>(std::forward<T>(t)))
            , type_index_(typeid(std::decay<T>::type))
        {
        }

        template <class T>
        auto is() -> bool
        {
            return type_index_ == std::type_index(typeid(T));
        }

        template <class T>
        auto cast() -> T&
        {
            return ptr_ ? nullptr : is<T>() ? &static_cast<detail::derived<T>*>(ptr_.get())->value_
                                            : nullptr;
        }

    private:
        auto clone() const -> uptr<detail::base>;
    };

} // namespace util
} // namespace hare

#endif // _HARE_BASE_UTIL_ANY_H_