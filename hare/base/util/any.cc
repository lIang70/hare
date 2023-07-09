#include <hare/base/util/any.h>

namespace hare {
namespace util {

    any::any()
        : type_index_(std::type_index(typeid(void)))
    {
    }

    any::any(const any& other)
        : ptr_(other.clone())
        , type_index_(other.type_index_)
    {
    }

    any::any(any&& other) noexcept
        : ptr_(std::move(other.ptr_))
        , type_index_(other.type_index_)
    {
    }

    auto any::operator=(const any& a) -> any&
    {
        if (ptr_ == a.ptr_) {
            return *this;
        }
        ptr_ = a.clone();
        type_index_ = a.type_index_;
        return *this;
    }

    auto any::clone() const -> uptr<detail::base>
    {
        if (ptr_) {
            return ptr_->clone();
        }
        return nullptr;
    }

} // namespace util
} // namespace hare