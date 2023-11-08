#include "base/fwd_inl.h"
#include "base/io/buffer_inl.h"

namespace hare {

HARE_IMPL_DPTR(BufferIterator);

BufferIteratorImpl::BufferIteratorImpl(buffer_inner::CacheList* _list)
    : list(_list),
      iter(_list->Begin()),
      curr_index(::hare::detail::ToUnsigned((*_list->Begin())->Readable() -
                                            (*_list->Begin())->Data())) {}

BufferIteratorImpl::BufferIteratorImpl(const buffer_inner::CacheList* _list,
                                       buffer_inner::CacheList::Node* _iter)
    : list(_list), iter(_iter), curr_index((*_iter)->size()) {}

auto BufferIterator::operator*() noexcept -> char {
  return Valid() ? (*IMPL->iter)->Data()[IMPL->curr_index] : '\0';
}

auto BufferIterator::operator++() noexcept -> BufferIterator& {
  if (Valid()) {
    auto* end = IMPL->list->End();

    if (IMPL->iter == end && IMPL->curr_index == (*end)->size()) {
      return (*this);
    }

    ++IMPL->curr_index;
    if (IMPL->curr_index > (*IMPL->iter)->size()) {
      IMPL->iter = IMPL->iter->next;
      IMPL->curr_index = ::hare::detail::ToUnsigned((*IMPL->iter)->Readable() -
                                                    (*IMPL->iter)->Data());
    }
  }
  return (*this);
}

auto BufferIterator::operator--() noexcept -> BufferIterator& {
  if (Valid()) {
    auto* begin = IMPL->list->Begin();
    if (IMPL->iter == begin &&
        IMPL->curr_index == ::hare::detail::ToUnsigned((*begin)->Readable() -
                                                       (*begin)->Data())) {
      return (*this);
    }

    if (IMPL->curr_index == 0 ||
        IMPL->curr_index - 1 <
            ::hare::detail::ToUnsigned((*IMPL->iter)->Readable() -
                                       (*IMPL->iter)->Data())) {
      IMPL->iter = IMPL->iter->prev;
      IMPL->curr_index = (*IMPL->iter)->size();
    } else {
      --IMPL->curr_index;
    }
  }
  return (*this);
}

auto operator==(const BufferIterator& _x, const BufferIterator& _y) noexcept
    -> bool {
  return d_ptr(_x.impl_)->iter == d_ptr(_y.impl_)->iter &&
         d_ptr(_x.impl_)->curr_index == d_ptr(_y.impl_)->curr_index;
}

auto operator!=(const BufferIterator& _x, const BufferIterator& _y) noexcept
    -> bool {
  return d_ptr(_x.impl_)->iter != d_ptr(_y.impl_)->iter ||
         d_ptr(_x.impl_)->curr_index != d_ptr(_y.impl_)->curr_index;
}

}  // namespace hare