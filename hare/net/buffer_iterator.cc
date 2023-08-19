#include "base/fwd-inl.h"
#include "net/buffer-inl.h"

namespace hare {
namespace net {

    HARE_INLINE
    auto d_ptr(hare::detail::Impl* impl_)
        -> buffer_iterator_impl* { return down_cast<buffer_iterator_impl*>(impl_); }

    auto BufferIterator::operator*() noexcept -> char
    {
        return Valid() ? (*IMPL->iter)->Data()[IMPL->curr_index] : '\0';
    }

    auto BufferIterator::operator++() noexcept -> BufferIterator&
    {
        if (Valid()) {
            auto* end = IMPL->list->End();

            if (IMPL->iter == end && IMPL->curr_index == (*end)->size()) {
                return (*this);
            }

            ++IMPL->curr_index;
            if (IMPL->curr_index > (*IMPL->iter)->size()) {
                IMPL->iter = IMPL->iter->next;
                IMPL->curr_index = hare::detail::ToUnsigned((*IMPL->iter)->Readable() - (*IMPL->iter)->Data());
            }
        }
        return (*this);
    }

    auto BufferIterator::operator--() noexcept -> BufferIterator&
    {
        if (Valid()) {
            auto* begin = IMPL->list->Begin();
            if (IMPL->iter == begin && IMPL->curr_index == hare::detail::ToUnsigned((*begin)->Readable() - (*begin)->Data())) {
                return (*this);
            }

            if (IMPL->curr_index == 0 || IMPL->curr_index - 1 < hare::detail::ToUnsigned((*IMPL->iter)->Readable() - (*IMPL->iter)->Data())) {
                IMPL->iter = IMPL->iter->prev;
                IMPL->curr_index = (*IMPL->iter)->size();
            } else {
                --IMPL->curr_index;
            }
        }
        return (*this);
    }

    auto operator==(const BufferIterator& _x, const BufferIterator& _y) noexcept -> bool
    {
        return d_ptr(_x.impl_)->iter == d_ptr(_y.impl_)->iter && d_ptr(_x.impl_)->curr_index == d_ptr(_y.impl_)->curr_index;
    }

    auto operator!=(const BufferIterator& _x, const BufferIterator& _y) noexcept -> bool
    {
        return d_ptr(_x.impl_)->iter != d_ptr(_y.impl_)->iter || d_ptr(_x.impl_)->curr_index != d_ptr(_y.impl_)->curr_index;
    }

} // namespace net
} // namespace hare