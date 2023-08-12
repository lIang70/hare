#include "hare/net/buffer-inl.h"

namespace hare {
namespace net {

    HARE_INLINE
    auto d_ptr(hare::detail::Impl* impl_) 
        -> buffer_iterator_impl* { return down_cast<buffer_iterator_impl*>(impl_); }

    auto BufferIterator::operator*() noexcept -> char
    {
        return Valid() ? (*d_ptr(impl_)->iter)->Data()[d_ptr(impl_)->curr_index] : '\0';
    }

    auto BufferIterator::operator++() noexcept -> BufferIterator&
    {
        if (Valid()) {
            auto* end = d_ptr(impl_)->list->End();

            if (d_ptr(impl_)->iter == end && 
                d_ptr(impl_)->curr_index == (*end)->size()) {
                return (*this);
            }

            ++d_ptr(impl_)->curr_index;
            if (d_ptr(impl_)->curr_index > (*d_ptr(impl_)->iter)->size()) {
                d_ptr(impl_)->iter = d_ptr(impl_)->iter->next;
                d_ptr(impl_)->curr_index = 
                    hare::detail::ToUnsigned((*d_ptr(impl_)->iter)->Readable() - (*d_ptr(impl_)->iter)->Data());
            }
        }
        return (*this);
    }

    auto BufferIterator::operator--() noexcept -> BufferIterator&
    {
        if (Valid()) {
            auto* begin = d_ptr(impl_)->list->Begin();
            if (d_ptr(impl_)->iter == begin && 
                d_ptr(impl_)->curr_index == hare::detail::ToUnsigned((*begin)->Readable() - (*begin)->Data())) {
                return (*this);
            }

            if (d_ptr(impl_)->curr_index == 0 || 
                d_ptr(impl_)->curr_index - 1 < hare::detail::ToUnsigned((*d_ptr(impl_)->iter)->Readable() - (*d_ptr(impl_)->iter)->Data())) {
                d_ptr(impl_)->iter = d_ptr(impl_)->iter->prev;
                d_ptr(impl_)->curr_index = (*d_ptr(impl_)->iter)->size();
            } else {
                --d_ptr(impl_)->curr_index;
            }
        }
        return (*this);
    }

    auto operator==(const BufferIterator& _x, const BufferIterator& _y) noexcept -> bool
    {
        return d_ptr(_x.impl_)->iter == d_ptr(_y.impl_)->iter &&
            d_ptr(_x.impl_)->curr_index == d_ptr(_y.impl_)->curr_index;
    }

    auto operator!=(const BufferIterator& _x, const BufferIterator& _y) noexcept -> bool
    {
        return d_ptr(_x.impl_)->iter != d_ptr(_y.impl_)->iter ||
            d_ptr(_x.impl_)->curr_index != d_ptr(_y.impl_)->curr_index;
    }

} // namespace net
} // namespace hare