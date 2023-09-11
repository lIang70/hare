#include "net/socket_op.h"

#include <hare/hare-config.h>

#include "base/fwd-inl.h"

#ifdef H_OS_WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#define NO_ACCEPT4
#define socklen_t int
#endif

#if HARE__HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#if HARE__HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HARE__HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HARE__HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#define MAX_READ_DEFAULT 4096

namespace hare {
namespace socket_op {

namespace socket_op_inner {

#if defined(NO_ACCEPT4)
static void SetForNotAccept4(util_socket_t _fd) {
#ifndef H_OS_WIN
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
#else
  u_long noblock = 1;
  auto ret = ::ioctlsocket(_fd, FIONBIO, &noblock);
  // no need close-on-exec
  IgnoreUnused(ret);
#endif
}
#endif

}  // namespace socket_op_inner

auto CreateNonblockingOrDie(std::uint8_t _family) -> util_socket_t {
#ifndef H_OS_WIN
  auto _fd = ::socket(_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (_fd < 0) {
    HARE_INTERNAL_FATAL("cannot create non-blocking socket");
  }
#else
  auto _fd = ::socket(_family, SOCK_STREAM, 0);
  if (_fd < 0) {
    HARE_INTERNAL_FATAL("cannot create non-blocking socket");
  }
  detail::set_nonblock_closeonexec(_fd);
#endif
  return _fd;
}

auto Close(util_socket_t _fd) -> std::int32_t {
#ifndef H_OS_WIN
  auto ret = ::close(_fd);
  if (ret < 0) {
    HARE_INTERNAL_ERROR("close fd[{}], detail:{}.", _fd,
                        io::SocketErrorInfo(_fd));
  }
#else
  auto ret = ::closesocket(_fd);
  if (ret < 0) {
    HARE_INTERNAL_ERROR("close fd[{}], detail:{}.", _fd,
                        get_socket_error_info(_fd));
  }
#endif
  return ret;
}

auto Write(util_socket_t _fd, const void* _buf, std::size_t size)
    -> std::int64_t {
#ifndef H_OS_WIN
  return ::write(_fd, _buf, size);
#else
  return ::_write(_fd, _buf, size);
#endif
}

auto Read(util_socket_t _fd, void* _buf, std::size_t size) -> std::int64_t {
#ifndef H_OS_WIN
  return ::read(_fd, _buf, size);
#else
  return ::_read(_fd, _buf, size);
#endif
}

auto Bind(util_socket_t _fd, const struct sockaddr* _addr,
          std::size_t _addr_len) -> bool {
#ifndef H_OS_WIN
  return ::bind(_fd, _addr, _addr_len) != -1;
#else
  return ::bind(_fd, _addr, _addr_len) != -1;
#endif
}

auto Listen(util_socket_t _fd) -> bool {
#ifndef H_OS_WIN
  return ::listen(_fd, SOMAXCONN) >= 0;
#else
  return ::listen(_fd, SOMAXCONN) >= 0;
#endif
}

auto Connect(util_socket_t _fd, const struct sockaddr* _addr,
             std::size_t _addr_len) -> bool {
#ifndef H_OS_WIN
  return ::connect(_fd, _addr, _addr_len) >= 0;
#else
  return ::connect(_fd, _addr, (socklen_t)_addr_len) >= 0;
#endif
}

auto Accept(util_socket_t _fd, struct sockaddr* _addr, std::size_t _addr_len)
    -> util_socket_t {
#if defined(NO_ACCEPT4)
  auto accept_fd = ::accept(_fd, _addr, (socklen_t*)&_addr_len);
  detail::SetForNotAccept4(accept_fd);
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
#if defined(FIONREAD) && defined(H_OS_WIN32)
  u_long lng = MAX_READ_DEFAULT;
  if (::ioctlsocket(_fd, FIONREAD, &lng) < 0) {
    return (0);
  }
  /* Can overflow, but mostly harmlessly. XXXX */
  return lng;
#elif defined(FIONREAD)
  auto reable_cnt = MAX_READ_DEFAULT;
  if (::ioctl(_fd, FIONREAD, &reable_cnt) < 0) {
    return (0);
  }
  return reable_cnt;
#else
  return MAX_READ_DEFAULT;
#endif
}

void ToIpPort(char* _buf, std::size_t size, const struct sockaddr* _addr) {
  if (_addr->sa_family == AF_INET6) {
    _buf[0] = '[';
    ToIp(_buf + 1, size - 1, _addr);
    auto end = ::strlen(_buf);
    const auto* addr6 = SockaddrCastIn6(_addr);
    auto port = io::NetworkToHost16(addr6->sin6_port);
    HARE_ASSERT(size > end);
    IgnoreUnused(::snprintf(_buf + end, size - end, "]:%u", port));
    return;
  }
  ToIp(_buf, size, _addr);
  auto end = ::strlen(_buf);
  const auto* addr4 = SockaddrCastIn(_addr);
  auto port = io::NetworkToHost16(addr4->sin_port);
  HARE_ASSERT(size > end);
  IgnoreUnused(::snprintf(_buf + end, size - end, ":%u", port));
}

void ToIp(char* _buf, std::size_t size, const struct sockaddr* _addr) {
  if (_addr->sa_family == AF_INET) {
    HARE_ASSERT(size >= INET_ADDRSTRLEN);
    const auto* addr4 = SockaddrCastIn(_addr);
    ::inet_ntop(AF_INET, &addr4->sin_addr, _buf, static_cast<socklen_t>(size));
  } else if (_addr->sa_family == AF_INET6) {
    HARE_ASSERT(size >= INET6_ADDRSTRLEN);
    const auto* addr6 = SockaddrCastIn6(_addr);
    ::inet_ntop(AF_INET6, &addr6->sin6_addr, _buf,
                static_cast<socklen_t>(size));
  }
}

auto FromIpPort(const char* target_ip, std::uint16_t port,
                struct sockaddr_in* _addr) -> bool {
  _addr->sin_family = AF_INET;
  _addr->sin_port = io::HostToNetwork16(port);
  return ::inet_pton(AF_INET, target_ip, &_addr->sin_addr) <= 0;
}

auto FromIpPort(const char* target_ip, std::uint16_t port,
                struct sockaddr_in6* _addr) -> bool {
  _addr->sin6_family = AF_INET6;
  _addr->sin6_port = io::HostToNetwork16(port);
  return ::inet_pton(AF_INET6, target_ip, &_addr->sin6_addr) <= 0;
}

}  // namespace socket_op
}  // namespace hare