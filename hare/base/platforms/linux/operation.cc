#include "base/io/operation_inl.h"

#ifdef H_OS_LINUX
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

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
  auto _fd = ::socket(_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (_fd < 0) {
    HARE_INTERNAL_FATAL("cannot create non-blocking socket");
  }
  return _fd;
}

auto Close(util_socket_t _fd) -> std::int32_t {
  auto ret = ::close(_fd);
  if (ret < 0) {
    HARE_INTERNAL_ERROR("close fd[{}], detail:{}.", _fd,
                        io::SocketErrorInfo(_fd));
  }
  return ret;
}

auto Write(util_socket_t _fd, const void* _buf, std::size_t size)
    -> std::int64_t {
  return ::write(_fd, _buf, size);
}

auto Read(util_socket_t _fd, void* _buf, std::size_t size) -> std::int64_t {
  return ::read(_fd, _buf, size);
}

auto Bind(util_socket_t _fd, const struct sockaddr* _addr,
          std::size_t _addr_len) -> bool {
  return ::bind(_fd, _addr, _addr_len) != -1;
}

auto Listen(util_socket_t _fd) -> bool { return ::listen(_fd, SOMAXCONN) >= 0; }

auto Connect(util_socket_t _fd, const struct sockaddr* _addr,
             std::size_t _addr_len) -> bool {
  return ::connect(_fd, _addr, _addr_len) >= 0;
}

auto Accept(util_socket_t _fd, struct sockaddr* _addr, std::size_t _addr_len)
    -> util_socket_t {
#if defined(NO_ACCEPT4)
  auto accept_fd = ::accept(_fd, _addr, (socklen_t*)&_addr_len);
  io_inner::SetForNotAccept4(accept_fd);
#else
  auto accept_fd = ::accept4(_fd, _addr, (socklen_t*)&_addr_len,
                             SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
  if (accept_fd < 0) {
    auto saved_errno = errno;
    switch (saved_errno) {
      case EAGAIN:
      case ECONNABORTED:
      case EINTR:
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