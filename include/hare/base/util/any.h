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
        struct HARE_API Base {
            using UPtr = UPtr<Base>;

            virtual ~Base() = default;
            virtual auto Clone() const -> UPtr = 0;
        };

        HARE_CLASS_API
        template <typename T>
        struct HARE_API Derived : public Base {
            T value_;

            template <typename... Args>
            explicit Derived(Args&&... args)
                : value_(std::forward<Args>(args)...)
            {
            }

            auto Clone() const -> UPtr override
            {
                return uptr(new Derived(value_));
            }
        };

    } // namespace detail

    HARE_CLASS_API
    class HARE_API Any {
        detail::Base::UPtr ptr_ {};
        std::type_index type_index_;

    public:
        HARE_INLINE
        Any() : type_index_(std::type_index(typeid(void)))
        {}

        HARE_INLINE
        Any(const Any& other)
            : ptr_(other.Clone())
            , type_index_(other.type_index_)
        {}

        HARE_INLINE
        Any(Any&& other) noexcept
            : ptr_(std::move(other.ptr_))
            , type_index_(other.type_index_)
        { }

        HARE_INLINE
        auto operator=(const Any& a) -> Any&
        {
            if (ptr_ == a.ptr_) {
                return *this;
            }
            ptr_ = a.Clone();
            type_index_ = a.type_index_;
            return *this;
        }

        HARE_INLINE auto IsNull() const -> bool { return !ptr_; }

        template <class T, class = typename std::enable_if<!std::is_same<typename std::decay<T>::type, Any>::value, T>::type>
        HARE_INLINE
        explicit Any(T&& t)
            : ptr_(new detail::Derived<typename std::decay<T>::type>(std::forward<T>(t)))
            , type_index_(typeid(std::decay<T>::type))
        {
        }

        template <class T>
        HARE_INLINE
        auto Is() const -> bool
        {
            return type_index_ == std::type_index(typeid(T));
        }

        template <class T>
        HARE_INLINE
        auto Cast() const -> T&
        {
            return ptr_ ? nullptr : Is<T>() ? &static_cast<detail::Derived<T>*>(ptr_.get())->value_
                                            : nullptr;
        }

    private:
        HARE_INLINE
        auto Clone() const -> UPtr<detail::Base>
        {
            if (ptr_) {
                return ptr_->Clone();
            }
            return nullptr;
        }
    };

} // namespace util
} // namespace hare

#endif // _HARE_BASE_UTIL_ANY_H_
