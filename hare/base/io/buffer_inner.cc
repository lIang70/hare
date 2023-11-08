#include "base/io/buffer_inl.h"

namespace hare {

namespace buffer_inner {

static auto RoundUp(std::size_t _size) -> std::size_t {
  std::size_t size{MIN_TO_ALLOC};
  if (_size < MAX_SIZE / 2) {
    while (size < _size) {
      size <<= 1;
    }
  } else {
    size = _size;
  }
  return size;
}

auto Cache::Realign(std::size_t _size) -> bool {
  auto offset = ReadableSize();
  if (WriteableSize() >= _size) {
    return true;
  } else if (WriteableSize() + misalign_ > _size && offset <= MAX_TO_REALIGN) {
    ::memmove(Begin(), Readable(), offset);
    Set(Begin(), offset);
    misalign_ = 0;
    return true;
  }
  return false;
}

void CacheList::CheckSize(std::size_t _size) {
  if (!write->cache) {
    write->cache.reset(new Cache(RoundUp(_size)));
  } else if (!(*write)->Realign(_size)) {
    GetNextWrite();
    if (!write->cache || (*write)->size() < _size) {
      write->cache.reset(new Cache(RoundUp(_size)));
    }
  }
}

void CacheList::Append(CacheList& _other) {
  auto* other_begin = _other.Begin();
  auto* other_end = _other.End();
  auto chain_size = 1;

  while (other_begin != other_end) {
    ++chain_size;
    other_begin = other_begin->next;
  }
  other_begin = _other.Begin();

  if (other_begin->prev == other_end) {
    //   ---+---++---+---
    // .... |   ||   | ....
    //   ---+---++---+---
    //        ^    ^
    //      write read
    End()->next = other_begin;
    other_begin->prev = End();
    Begin()->prev = other_end;
    other_end->next = Begin();
    write = other_end;

    // reset other
    _other.node_size_ = 1;
    _other.head = new buffer_inner::CacheList::Node;
    _other.head->next = _other.head;
    _other.head->prev = _other.head;
    _other.read = _other.head;
    _other.write = _other.head;
  } else {
    //   ---+---++-   --++---+---
    // .... |   || ...  ||   | ....
    //   ---+---++-   --++---+---
    //        ^            ^
    //      write         read
    auto* before_begin = other_begin->prev;
    auto* after_end = other_end->next;
    End()->next = other_begin;
    other_begin->prev = End();
    Begin()->prev = other_end;
    other_end->next = Begin();
    write = other_end;

    // chained lists are re-formed into rings
    before_begin->next = after_end;
    after_end->prev = before_begin;
    _other.node_size_ -= chain_size;
    _other.head = after_end;
    _other.read = _other.head;
    _other.write = _other.head;
  }

  node_size_ += chain_size;
}

auto CacheList::FastExpand(std::size_t _size) -> std::int32_t {
  auto cnt{0};
  GetNextWrite();
  auto* index = End();
  if (!index->cache) {
    index->cache.reset(new Cache(Min(RoundUp(_size), MAX_TO_ALLOC)));
  }

  do {
    _size -= Min((*index)->WriteableSize(), _size);
    ++cnt;
    if (index->next == read) {
      break;
    }
    index = index->next;
  } while (true);

  while (_size > 0) {
    auto alloc_size = Min(RoundUp(_size), MAX_TO_ALLOC);
    auto* tmp = new Node;
    tmp->cache.reset(new Cache(alloc_size));
    tmp->prev = index;
    tmp->next = index->next;
    index->next->prev = tmp;
    index->next = tmp;
    index = index->next;
    ++node_size_;
    ++cnt;
  }
  return cnt;
}

// The legality of size is checked before use
void CacheList::Add(std::size_t _size) {
  auto* index = End();
  while (_size > 0) {
    auto write_size = Min(_size, (*index)->WriteableSize());
    (*index)->Add(write_size);
    _size -= write_size;
    if ((*index)->Full() && index->next != read) {
      index = index->next;
    } else {
      HARE_ASSERT(_size == 0);
    }
  }
  write = index;
}

// The legality of size is checked before use
void CacheList::Drain(std::size_t _size) {
  auto* index = Begin();
  auto need_drain{false};
  do {
    auto drain_size = Min(_size, (*index)->ReadableSize());
    _size -= drain_size;
    (*index)->Drain(drain_size);
    if ((*index)->Empty()) {
      (*index)->Clear();

      if (index != End()) {
        need_drain = true;
        index = index->next;
      }
    }
  } while (_size != 0 && index != End());

  if (_size > 0) {
    HARE_ASSERT(_size <= (*index)->ReadableSize());
    (*index)->Drain(_size);
    _size = 0;
  }

  /**
   * @brief If the length of chain bigger than MAX_CHINA_SIZE,
   *   cleaning will begin.
   **/
  if (need_drain && node_size_ > MAX_CHINA_SIZE) {
    while (Begin()->next != index) {
      auto* tmp = Begin()->next;
      tmp->next->prev = Begin();
      Begin()->next = tmp->next;
      delete tmp;
      --node_size_;
    }
  }
  read = index;
}

void CacheList::Reset() {
  while (head->next != head) {
    auto* tmp = head->next;
    tmp->next->prev = head;
    head->next = tmp->next;
    delete tmp;
  }
  if (head->cache) {
    (*head)->Clear();
  }
  node_size_ = 1;
  read = head;
  write = head;
}

void CacheList::PrintStatus(const std::string& _status) const {
  HARE_INTERNAL_TRACE("[{}] list total length: {:}", _status, node_size_);

#ifdef HARE_DEBUG
  auto* index = Begin();
  do {
    /// | (RW|R|W|E|N) ... | [<-(w_itre|r_iter)]
    const auto* mark =
        !index->cache                                                   ? "(N)"
        : (*index)->ReadableSize() > 0 && (*index)->WriteableSize() > 0 ? "(RW)"
        : (*index)->ReadableSize() > 0                                  ? "(R)"
        : (*index)->WriteableSize() > 0                                 ? "(W)"
                                                                        : "(E)";

    HARE_INTERNAL_TRACE("|{:4} {} {} {}|{}", mark,
                        !index->cache ? 0 : (*index)->ReadableSize(),
                        !index->cache ? 0 : (*index)->WriteableSize(),
                        !index->cache ? 0 : (*index)->capacity(),
                        index == read && index == write ? " <- w_iter|r_iter"
                        : index == read                 ? " <- r_iter"
                        : index == write                ? " <- w_iter"
                                                        : "");
    index = index->next;
  } while (index != Begin());
#endif

  HARE_INTERNAL_TRACE("{:^16}", "*");
}

}  // namespace buffer_inner

}  // namespace hare