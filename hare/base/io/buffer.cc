#include <hare/base/exception.h>

#include <vector>

#include "base/fwd_inl.h"
#include "base/io/buffer_inl.h"

namespace hare {

namespace buffer_inner {

// kmp
static auto GetNext(const char* _begin, std::size_t _size)
    -> std::vector<std::int32_t> {
  std::vector<std::int32_t> next(_size, -1);
  std::int32_t k = -1;
  for (std::size_t j = 0; j < _size;) {
    if (k == -1 || _begin[j] == _begin[k]) {
      ++j, ++k;
      if (_begin[j] != _begin[k]) {
        next[j] = k;
      } else {
        next[j] = next[k];
      }
    } else {
      k = next[k];
    }
  }
  return next;
}

}  // namespace buffer_inner

HARE_IMPL_DPTR(Buffer);

Buffer::Buffer(std::size_t _max_read) : impl_(new BufferImpl) {
  IMPL->max_read = _max_read;
}

Buffer::~Buffer() { delete impl_; }

auto Buffer::Size() const -> std::size_t { return IMPL->total_len; }

void Buffer::SetMaxRead(std::size_t _max_read) { IMPL->max_read = _max_read; }

void Buffer::ClearAll() {
  IMPL->cache_chain.Reset();
  IMPL->total_len = 0;
}

void Buffer::Skip(std::size_t _size) {
  if (_size >= IMPL->total_len) {
    ClearAll();
  } else {
    IMPL->total_len -= _size;
    IMPL->cache_chain.Drain(_size);
  }
}

auto Buffer::Begin() -> Iterator {
  return Iterator(new BufferIteratorImpl(&IMPL->cache_chain));
}

auto Buffer::End() -> Iterator {
  return Iterator(
      new BufferIteratorImpl(&IMPL->cache_chain, IMPL->cache_chain.End()));
}

auto Buffer::Find(const char* _begin, std::size_t _size) -> Iterator {
  if (_size > IMPL->total_len) {
    return End();
  }
  auto next_val = buffer_inner::GetNext(_begin, _size);
  auto i{Begin()};
  auto j{0};
  while (i != End() && (j == -1 || (std::size_t)j < _size)) {
    if (j == -1 || (*i) == _begin[j]) {
      ++i, ++j;
    } else {
      j = next_val[j];
    }
  }

  if (j >= 0 && (std::size_t)j >= _size) {
    return i;
  }
  return End();
}

void Buffer::Append(Buffer& _other) {
  if (d_ptr(_other.impl_)->total_len == 0 || &_other == this) {
    return;
  }

  IMPL->cache_chain.PrintStatus("before append buffer");
  d_ptr(_other.impl_)->cache_chain.PrintStatus("before append other buffer");

  if (IMPL->total_len == 0) {
    IMPL->cache_chain.Swap(d_ptr(_other.impl_)->cache_chain);
  } else {
    IMPL->cache_chain.Append(d_ptr(_other.impl_)->cache_chain);
  }

  IMPL->cache_chain.PrintStatus("after append buffer");
  d_ptr(_other.impl_)->cache_chain.PrintStatus("after append other buffer");

  IMPL->total_len += d_ptr(_other.impl_)->total_len;
  d_ptr(_other.impl_)->total_len = 0;
}

auto Buffer::Add(const void* _bytes, std::size_t _size) -> bool {
  if (IMPL->total_len + _size > MAX_SIZE) {
    return false;
  }

  IMPL->total_len += _size;
  IMPL->cache_chain.CheckSize(_size);
  IMPL->cache_chain.End()->cache->Append(
      static_cast<const char*>(_bytes),
      static_cast<const char*>(_bytes) + _size);

  IMPL->cache_chain.PrintStatus("after add");
  return true;
}

auto Buffer::Remove(void* _buffer, std::size_t _length) -> std::size_t {
  using ::hare::util_inner::MakeChecked;

  if (_length == 0 || IMPL->total_len == 0) {
    return 0;
  }

  if (_length > IMPL->total_len) {
    _length = IMPL->total_len;
  }

  auto* curr{IMPL->cache_chain.Begin()};
  std::size_t total{0};
  auto* dest = static_cast<char*>(_buffer);
  while (total < _length) {
    auto copy_len = Min(_length - total, (*curr)->ReadableSize());
    std::uninitialized_copy_n(dest + total, copy_len,
                              MakeChecked((*curr)->Readable(), copy_len));
    total += copy_len;
    curr = curr->next;
  }

  IMPL->total_len -= _length;
  IMPL->cache_chain.Drain(_length);

  IMPL->cache_chain.PrintStatus("after remove");
  return _length;
}

void Buffer::Move(Buffer& _other) noexcept {
  IMPL->cache_chain.Swap(d_ptr(_other.impl_)->cache_chain);
  std::swap(IMPL->total_len, d_ptr(_other.impl_)->total_len);
  std::swap(IMPL->max_read, d_ptr(_other.impl_)->max_read);
}

}  // namespace hare