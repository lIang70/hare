/**
 * @file hare/net/buffer.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with buffer.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_BUFFER_H_
#define _HARE_NET_BUFFER_H_

#include <hare/base/util/non_copyable.h>

#define HARE_MAX_READ_DEFAULT 4096

namespace hare {
namespace net {

class Buffer;

HARE_CLASS_API
class HARE_API BufferIterator : public util::NonCopyable {
  hare::detail::Impl* impl_{};

 public:
  HARE_INLINE
  BufferIterator(BufferIterator&& _other) noexcept { Move(_other); }

  HARE_INLINE
  auto operator=(BufferIterator&& _other) noexcept -> BufferIterator& {
    Move(_other);
    return (*this);
  }

  HARE_INLINE
  auto Valid() const -> bool { return impl_ != nullptr; }

  auto operator*() noexcept -> char;
  auto operator++() noexcept -> BufferIterator&;
  auto operator--() noexcept -> BufferIterator&;

  friend auto operator==(const BufferIterator& _x,
                         const BufferIterator& _y) noexcept -> bool;
  friend auto operator!=(const BufferIterator& _x,
                         const BufferIterator& _y) noexcept -> bool;

 private:
  HARE_INLINE
  explicit BufferIterator(hare::detail::Impl* _impl) : impl_(_impl) {}

  HARE_INLINE
  void Move(BufferIterator& _other) noexcept { std::swap(impl_, _other.impl_); }

  friend class net::Buffer;
};

HARE_CLASS_API
class HARE_API Buffer : public util::NonCopyable {
  hare::detail::Impl* impl_{};

 public:
  using Iterator = BufferIterator;

  explicit Buffer(std::size_t _max_read = HARE_MAX_READ_DEFAULT);
  ~Buffer();

  HARE_INLINE
  Buffer(Buffer&& _other) noexcept { Move(_other); }

  HARE_INLINE
  auto operator=(Buffer&& _other) noexcept -> Buffer& {
    Move(_other);
    return (*this);
  }

  auto Size() const -> std::size_t;
  void SetMaxRead(std::size_t _max_read);
  auto ChainSize() const -> std::size_t;
  void ClearAll();
  void Skip(std::size_t _size);

  auto Begin() -> Iterator;
  auto End() -> Iterator;
  auto Find(const char* _begin, std::size_t _size) -> Iterator;

  // read-write
  void Append(Buffer& _other);

  auto Add(const void* _bytes, std::size_t _size) -> bool;
  auto Remove(void* _buffer, std::size_t _length) -> std::size_t;

  auto Read(util_socket_t _fd, std::size_t _howmuch) -> std::size_t;
  auto Write(util_socket_t _fd, std::size_t _howmuch = 0) -> std::size_t;

 private:
  void Move(Buffer& _other) noexcept;
};

}  // namespace net
}  // namespace hare

#endif  // _HARE_NET_BUFFER_H_