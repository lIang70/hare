#include <hare/base/io/operation.h>
#include <hare/hare-config.h>

#ifdef H_OS_WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#define socklen_t int
#define close closesocket
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define MAX_READ_DEFAULT 4096

namespace hare {
namespace io {

namespace io_inner {

HARE_INLINE auto SockaddrCast(const struct sockaddr_in6* _addr) -> const
    struct sockaddr* {
  return static_cast<const struct sockaddr*>(ImplicitCast<const void*>(_addr));
}
HARE_INLINE auto SockaddrCast(struct sockaddr_in6* _addr) -> struct sockaddr* {
  return static_cast<struct sockaddr*>(ImplicitCast<void*>(_addr));
}
HARE_INLINE auto SockaddrCast(const struct sockaddr_in* _addr) -> const
    struct sockaddr* {
  return static_cast<const struct sockaddr*>(ImplicitCast<const void*>(_addr));
}
HARE_INLINE auto SockaddrCast(struct sockaddr_in* _addr) -> struct sockaddr* {
  return static_cast<struct sockaddr*>(ImplicitCast<void*>(_addr));
}

static auto CreatePair(int _family, int _type, int _protocol,
                       util_socket_t _fd[2]) -> std::int32_t {
  /* This socketpair does not work when localhost is down. So
   * it's really not the same thing at all. But it's close enough
   * for now, and really, when localhost is down sometimes, we
   * have other problems too.
   */
#define SET_SOCKET_ERROR(errcode) \
  do {                            \
    errno = (errcode);            \
  } while (0)
#ifdef H_OS_WIN
#define ERR(e) WSA##e
#else
#define ERR(e) e
#endif
  util_socket_t listener = -1;
  util_socket_t connector = -1;
  util_socket_t acceptor = -1;
  struct sockaddr_in listen_addr {};
  struct sockaddr_in connect_addr {};
  socklen_t size{};
  std::int32_t saved_errno = -1;

  auto family_test = _family != AF_INET;
#ifdef AF_UNIX
  family_test = family_test && (_type != AF_UNIX);
#endif
  if (_protocol || family_test) {
    SET_SOCKET_ERROR(ERR(EAFNOSUPPORT));
    return -1;
  }

  if (!_fd) {
    SET_SOCKET_ERROR(ERR(EINVAL));
    return -1;
  }

  listener = ::socket(AF_INET, _type, 0);
  if (listener < 0) {
    return -1;
  }

  hare::detail::FillN(&listen_addr, sizeof(listen_addr), 0);
  listen_addr.sin_family = AF_INET;
  listen_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  listen_addr.sin_port = 0;

  if (!::bind(listener, SockaddrCast(&listen_addr),
              static_cast<socklen_t>(sizeof(listen_addr)))) {
    goto tidy_up_and_fail;
  }
  if (::listen(listener, 1) == -1) {
    goto tidy_up_and_fail;
  }

  connector = ::socket(AF_INET, _type, 0);
  if (connector < 0) {
    goto tidy_up_and_fail;
  }

  hare::detail::FillN(&connect_addr, sizeof(connect_addr), 0);

  /* We want to find out the port number to connect to.  */
  size = sizeof(connect_addr);
  if (::getsockname(listener, SockaddrCast(&connect_addr),
                    static_cast<socklen_t*>(&size)) == -1) {
    goto tidy_up_and_fail;
  }
  if (size != sizeof(connect_addr)) {
    goto abort_tidy_up_and_fail;
  }
  if (!::connect(connector, SockaddrCast(&connect_addr),
                 static_cast<socklen_t>(sizeof(connect_addr)))) {
    goto tidy_up_and_fail;
  }

  size = sizeof(listen_addr);
  acceptor = ::accept(listener, SockaddrCast(&listen_addr),
                      static_cast<socklen_t*>(&size));
  if (acceptor < 0) {
    goto tidy_up_and_fail;
  }
  if (size != sizeof(listen_addr)) {
    goto abort_tidy_up_and_fail;
  }
  /* Now check we are talking to ourself by matching port and host on the
     two sockets.	 */
  if (::getsockname(connector, SockaddrCast(&connect_addr), &size) == -1) {
    goto tidy_up_and_fail;
  }
  if (size != sizeof(connect_addr) ||
      listen_addr.sin_family != connect_addr.sin_family ||
      listen_addr.sin_addr.s_addr != connect_addr.sin_addr.s_addr ||
      listen_addr.sin_port != connect_addr.sin_port) {
    goto abort_tidy_up_and_fail;
  }
  ::close(listener);

  _fd[0] = connector;
  _fd[1] = acceptor;

  return 0;

abort_tidy_up_and_fail:
  saved_errno = ERR(ECONNABORTED);
tidy_up_and_fail:
  if (saved_errno < 0) {
    saved_errno = errno;
  }
  if (listener != -1) {
    ::close(listener);
  }
  if (connector != -1) {
    ::close(connector);
  }
  if (acceptor != -1) {
    ::close(acceptor);
  }

  SET_SOCKET_ERROR(saved_errno);
  return -1;
#undef ERR
#undef SET_SOCKET_ERROR
}
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
#ifndef H_OS_WIN32
  return ::socketpair(_family, _type, _protocol, _sockets);
#else
  return io_inner::CreatePair(_family, _type, _protocol, _sockets);
#endif
}

}  // namespace io
}  // namespace hare