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

        HARE_CLASS_API
        struct HARE_API base {
            using uptr = uptr<base>;

            virtual ~base() = default;
            virtual auto clone() const -> uptr = 0;
        };

        HARE_CLASS_API
        template <typename T>
        struct HARE_API derived : public base {
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

    HARE_CLASS_API
    class HARE_API any {
        detail::base::uptr ptr_ {};
        std::type_index type_index_;

    public:
        HARE_INLINE
        any() : type_index_(std::type_index(typeid(void)))
        {}

        HARE_INLINE
        any::any(const any& other)
            : ptr_(other.clone())
            , type_index_(other.type_index_)
        {}

        HARE_INLINE
        any(any&& other) noexcept
            : ptr_(std::move(other.ptr_))
            , type_index_(other.type_index_)
        { }

        HARE_INLINE
        auto operator=(const any& a) -> any&
        {
            if (ptr_ == a.ptr_) {
                return *this;
            }
            ptr_ = a.clone();
            type_index_ = a.type_index_;
            return *this;
        }

        HARE_INLINE auto is_null() const -> bool { return !ptr_; }

        template <class T, class = typename std::enable_if<!std::is_same<typename std::decay<T>::type, any>::value, T>::type>
        HARE_INLINE
        explicit any(T&& t)
            : ptr_(new detail::derived<typename std::decay<T>::type>(std::forward<T>(t)))
            , type_index_(typeid(std::decay<T>::type))
        {
        }

        template <class T>
        HARE_INLINE
        auto is() const -> bool
        {
            return type_index_ == std::type_index(typeid(T));
        }

        template <class T>
        HARE_INLINE
        auto cast() const -> T&
        {
            return ptr_ ? nullptr : is<T>() ? &static_cast<detail::derived<T>*>(ptr_.get())->value_
                                            : nullptr;
        }

    private:
        HARE_INLINE
        auto clone() const -> uptr<detail::base>
        {
            if (ptr_) {
                return ptr_->clone();
            }
            return nullptr;
        }
    };

} // namespace util
} // namespace hare

#endif // _HARE_BASE_UTIL_ANY_H_
