#include "base/fwd_inl.h"
#include "base/io/operation_inl.h"

#ifdef H_OS_LINUX
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_READ_DEFAULT 4096

#define EINTR_LOOP(ret, call) \
  do {                        \
    ret = call;               \
  } while (ret == -1 && errno == EINTR)

namespace hare {
namespace io {

namespace io_inner {

#if defined(NO_ACCEPT4)
static void SetForNotAccept4(util_socket_t _fd) {
  // non-block
  auto flags = ::fcntl(_fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  auto ret = ::fcntl(_fd, F_SETFL, flags);
  // FIXME check

  // close-on-exec
  flags = ::fcntl(_fd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(_fd, F_SETFD, flags);
  // FIXME check

  IgnoreUnused(ret);
}
#endif

}  // namespace io_inner

auto SocketErrorInfo(util_socket_t _fd) -> std::string {
  std::array<char, HARE_SMALL_BUFFER> error_str{};
  auto error_len = error_str.size();
  if (::getsockopt(_fd, SOL_SOCKET, SO_ERROR, error_str.data(),
                   reinterpret_cast<socklen_t*>(&error_len)) != -1) {
    return error_str.data();
  }
  return {};
}

auto Socketpair(std::uint8_t _family, std::int32_t _type,
                std::int32_t _protocol, util_socket_t _sockets[2])
    -> std::int32_t {
  return ::socketpair(_family, _type, _protocol, _sockets);
}

auto CreateNonblockingOrDie(std::uint8_t _family) -> util_socket_t {
  util_socket_t fd{};

  EINTR_LOOP(fd,
             ::socket(_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));
  if (fd < 0) {
    HARE_INTERNAL_FATAL("cannot create non-blocking socket");
  }
  return fd;
}

auto Close(util_socket_t _fd) -> std::int32_t {
  std::int32_t ret{};
  EINTR_LOOP(ret, ::close(_fd));
  if (ret < 0) {
    HARE_INTERNAL_ERROR("close fd[{}], detail:{}.", _fd,
                        io::SocketErrorInfo(_fd));
  }
  return ret;
}

auto Write(util_socket_t _fd, const void* _buf, std::size_t size)
    -> std::int64_t {
  std::int64_t ret{};
  EINTR_LOOP(ret, ::write(_fd, _buf, size));
  return ret;
}

auto Read(util_socket_t _fd, void* _buf, std::size_t size) -> std::int64_t {
  std::int64_t ret{};
  EINTR_LOOP(ret, ::read(_fd, _buf, size));
  return ret;
}

auto Bind(util_socket_t _fd, const struct sockaddr* _addr,
          std::size_t _addr_len) -> bool {
  std::int32_t ret{};
  EINTR_LOOP(ret, ::bind(_fd, _addr, _addr_len));
  return ret != -1;
}

auto Listen(util_socket_t _fd) -> bool {
  std::int32_t ret{};
  EINTR_LOOP(ret, ::listen(_fd, SOMAXCONN));
  return ret >= 0;
}

auto Connect(util_socket_t _fd, const struct sockaddr* _addr,
             std::size_t _addr_len) -> bool {
  std::int32_t ret{};
  EINTR_LOOP(ret, ::connect(_fd, _addr, _addr_len));
  return ret >= 0;
}

auto Accept(util_socket_t _fd, struct sockaddr* _addr, std::size_t _addr_len)
    -> util_socket_t {
  util_socket_t accept_fd{};
#if defined(NO_ACCEPT4)
  EINTR_LOOP(accept_fd, ::accept(_fd, _addr, (socklen_t*)&_addr_len));
  io_inner::SetForNotAccept4(accept_fd);
#else
  EINTR_LOOP(accept_fd, ::accept4(_fd, _addr, (socklen_t*)&_addr_len,
                                  SOCK_NONBLOCK | SOCK_CLOEXEC));
#endif
  if (accept_fd < 0) {
    auto saved_errno = errno;
    switch (saved_errno) {
      case EAGAIN:
      case ECONNABORTED:
      case EPROTO:  // ???
      case EPERM:
      case EMFILE:  // per-process lmit of open file desctiptor ???
        // expected errors
        errno = saved_errno;
        break;
      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        // unexpected errors
        HARE_INTERNAL_FATAL("unexpected error of ::accept {}.", saved_errno);
        break;
      default:
        HARE_INTERNAL_FATAL("unknown error of ::accept {}.", saved_errno);
        break;
    }
  }
  return accept_fd;
}

auto AddrLen(std::uint8_t _family) -> std::size_t {
  return _family == PF_INET ? sizeof(struct sockaddr_in)
                            : sizeof(struct sockaddr_in6);
}

auto GetBytesReadableOnSocket(util_socket_t _fd) -> std::size_t {
#if defined(FIONREAD)
  auto reable_cnt = MAX_READ_DEFAULT;
  if (::ioctl(_fd, FIONREAD, &reable_cnt) < 0) {
    return (0);
  }
  return reable_cnt;
#else
  return MAX_READ_DEFAULT;
#endif
}

}  // namespace io
}  // namespace hare

#endif