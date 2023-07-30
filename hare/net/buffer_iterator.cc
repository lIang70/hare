#include "hare/net/buffer-inl.h"

namespace hare {
namespace net {

    HARE_INLINE
    auto d_ptr(hare::detail::impl* impl_) 
        -> buffer_iterator_impl* { return down_cast<buffer_iterator_impl*>(impl_); }

    auto buffer_iterator::operator*() noexcept -> char
    {
        return valid() ? (*d_ptr(impl_)->iter_)->data()[d_ptr(impl_)->curr_index_] : '\0';
    }

    auto buffer_iterator::operator++() noexcept -> buffer_iterator&
    {
        if (valid()) {
            auto* end = d_ptr(impl_)->list_->end();

            if (d_ptr(impl_)->iter_ == end && 
                d_ptr(impl_)->curr_index_ == (*end)->size()) {
                return (*this);
            }

            ++d_ptr(impl_)->curr_index_;
            if (d_ptr(impl_)->curr_index_ > (*d_ptr(impl_)->iter_)->size()) {
                d_ptr(impl_)->iter_ = d_ptr(impl_)->iter_->next;
                d_ptr(impl_)->curr_index_ = 
                    hare::detail::to_unsigned((*d_ptr(impl_)->iter_)->readable() - (*d_ptr(impl_)->iter_)->data());
            }
        }
        return (*this);
    }

    auto buffer_iterator::operator--() noexcept -> buffer_iterator&
    {
        if (valid()) {
            auto* begin = d_ptr(impl_)->list_->begin();
            if (d_ptr(impl_)->iter_ == begin && 
                d_ptr(impl_)->curr_index_ == hare::detail::to_unsigned((*begin)->readable() - (*begin)->data())) {
                return (*this);
            }

            if (d_ptr(impl_)->curr_index_ == 0 || 
                d_ptr(impl_)->curr_index_ - 1 < hare::detail::to_unsigned((*d_ptr(impl_)->iter_)->readable() - (*d_ptr(impl_)->iter_)->data())) {
                d_ptr(impl_)->iter_ = d_ptr(impl_)->iter_->prev;
                d_ptr(impl_)->curr_index_ = (*d_ptr(impl_)->iter_)->size();
            } else {
                --d_ptr(impl_)->curr_index_;
            }
        }
        return (*this);
    }

    auto operator==(const buffer_iterator& _x, const buffer_iterator& _y) noexcept -> bool
    {
        return d_ptr(_x.impl_)->iter_ == d_ptr(_y.impl_)->iter_ &&
            d_ptr(_x.impl_)->curr_index_ == d_ptr(_y.impl_)->curr_index_;
    }

    auto operator!=(const buffer_iterator& _x, const buffer_iterator& _y) noexcept -> bool
    {
        return d_ptr(_x.impl_)->iter_ != d_ptr(_y.impl_)->iter_ ||
            d_ptr(_x.impl_)->curr_index_ != d_ptr(_y.impl_)->curr_index_;
    }

} // namespace net
} // namespace hare