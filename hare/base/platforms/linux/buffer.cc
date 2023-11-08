#include <hare/base/io/buffer.h>
#include <hare/hare-config.h>

#include <vector>

#include "base/fwd_inl.h"
#include "base/io/buffer_inl.h"
#include "base/io/operation_inl.h"

#ifdef H_OS_LINUX

#if HARE__HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#define USE_IOVEC_IMPL
#endif

#if HARE__HAVE_SYS_UIO_H
#include <sys/uio.h>
#define DEFAULT_WRITE_IOVEC 128
#if defined(UIO_MAXIOV) && UIO_MAXIOV < DEFAULT_WRITE_IOVEC
#define NUM_WRITE_IOVEC UIO_MAXIOV
#elif defined(IOV_MAX) && IOV_MAX < DEFAULT_WRITE_IOVEC
#define NUM_WRITE_IOVEC IOV_MAX
#else
#define NUM_WRITE_IOVEC DEFAULT_WRITE_IOVEC
#endif
#define IOV_TYPE struct iovec
#define IOV_PTR_FIELD iov_base
#define IOV_LEN_FIELD iov_len
#define IOV_LEN_TYPE std::size_t
#else
#error "cannot use iovec in this system."
#endif

namespace hare {

HARE_IMPL_DPTR(Buffer);

auto Buffer::Read(util_socket_t _fd, std::size_t _howmuch) -> std::size_t {
  auto readable = ::hare::io::GetBytesReadableOnSocket(_fd);
  if (readable == 0) {
    readable = IMPL->max_read;
  }
  if (_howmuch == 0 || _howmuch > readable) {
    _howmuch = readable;
  } else {
    readable = _howmuch;
  }

  IMPL->cache_chain.PrintStatus("before read");

#ifdef USE_IOVEC_IMPL
  auto block_size = IMPL->cache_chain.FastExpand(readable);
  std::vector<IOV_TYPE> vecs(block_size);
  auto* block = IMPL->cache_chain.End();

  for (auto i = 0; i < block_size; ++i, block = block->next) {
    vecs[i].IOV_PTR_FIELD = (*block)->Writeable();
    vecs[i].IOV_LEN_FIELD = (*block)->WriteableSize();
  }

  {
    std::int64_t actual{};
    actual = ::readv(_fd, vecs.data(), block_size);
    readable = actual > 0 ? actual : 0;
    if (actual <= 0) {
      return (0);
    }
  }

  IMPL->cache_chain.Add(readable);
  IMPL->total_len += readable;

#else
#error "cannot use IOVEC."
#endif

  IMPL->cache_chain.PrintStatus("after read");

  return readable;
}

auto Buffer::Write(util_socket_t _fd, std::size_t _howmuch) -> std::size_t {
  std::size_t write_n{};
  auto total = _howmuch == 0 ? IMPL->total_len : _howmuch;

  if (total > IMPL->total_len) {
    total = IMPL->total_len;
  }

  IMPL->cache_chain.PrintStatus("before write");

#ifdef USE_IOVEC_IMPL
  std::array<IOV_TYPE, NUM_WRITE_IOVEC> iov{};
  auto* curr{IMPL->cache_chain.Begin()};
  auto write_i{0};

  while (write_i < NUM_WRITE_IOVEC && total > 0) {
    iov[write_i].IOV_PTR_FIELD = (*curr)->Readable();
    if (total > (*curr)->ReadableSize()) {
      /* XXXcould be problematic when windows supports mmap*/
      iov[write_i++].IOV_LEN_FIELD =
          static_cast<IOV_LEN_TYPE>((*curr)->ReadableSize());
      total -= (*curr)->ReadableSize();
      curr = curr->next;
    } else {
      /* XXXcould be problematic when windows supports mmap*/
      iov[write_i++].IOV_LEN_FIELD = static_cast<IOV_LEN_TYPE>(total);
      total = 0;
    }
  }

  if (write_i == 0) {
    return (0);
  }

  {
    std::int64_t actual{};
    actual = ::writev(_fd, iov.data(), write_i);
    write_n = actual > 0 ? actual : 0;
    if (actual <= 0) {
      return (0);
    }
  }

  IMPL->total_len -= write_n;
  IMPL->cache_chain.Drain(write_n);

#else
#error "cannot use IOVEC."
#endif

  IMPL->cache_chain.PrintStatus("after write");

  return write_n;
}

}  // namespace hare

#endif