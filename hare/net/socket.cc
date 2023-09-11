#include <hare/base/exception.h>
#include <hare/hare-config.h>
#include <hare/net/socket.h>

#include "base/fwd-inl.h"
#include "socket_op.h"

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HARE__HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

#if defined(H_OS_WIN)
#include <WinSock2.h>
#include <Ws2tcpip.h>
#define socklen_t int
#endif

namespace hare {
namespace net {

Socket::Socket(std::uint8_t family, Type type, util_socket_t socket)
    : socket_(socket), family_(family), type_(type) {
  if (socket == -1) {
    switch (type) {
      case TYPE_TCP:
        socket_ = socket_op::CreateNonblockingOrDie(family);
        break;
      case TYPE_INVALID:
      default:
        HARE_INTERNAL_FATAL("unrecognized type of socket.");
    }
  }
}

Socket::~Socket() { Close(); }

auto Socket::BindAddress(const HostAddress& local_addr) const -> Error {
  return socket_op::Bind(socket_, local_addr.get_sockaddr(),
                         socket_op::AddrLen(local_addr.Family()))
             ? Error()
             : Error(ERROR_SOCKET_BIND);
}

auto Socket::Listen() const -> Error {
  return socket_op::Listen(socket_) ? Error() : Error(ERROR_SOCKET_LISTEN);
}

auto Socket::Close() -> Error {
  if (socket_ != -1) {
    auto err = socket_op::Close(socket_);
    socket_ = -1;
    return err ? Error() : Error(ERROR_SOCKET_CLOSED);
  }
  return Error();
}

auto Socket::Accept(HostAddress& _peer_addr) const -> util_socket_t {
  struct sockaddr_in6 addr {};
  hare::detail::FillN(&addr, sizeof(addr), 0);
  auto accept_fd = socket_op::Accept(socket_, socket_op::SockaddrCast(&addr),
                                     socket_op::AddrLen(PF_INET6));
  if (accept_fd >= 0) {
    HARE_ASSERT(_peer_addr.in_ != nullptr);
    std::memcpy(_peer_addr.in_, &addr, sizeof(struct sockaddr_in6));
  }
  return accept_fd;
}

auto Socket::ShutdownWrite() const -> Error {
#ifndef H_OS_WIN
  return ::shutdown(socket_, SHUT_WR) < 0 ? Error(ERROR_SOCKET_SHUTDOWN_WRITE)
                                          : Error();
#else
  return ::shutdown(socket_, SD_SEND) < 0 ? error(ERROR_SOCKET_SHUTDOWN_WRITE)
                                          : error();
#endif
}

auto Socket::SetTcpNoDelay(bool _no_delay) const -> Error {
  auto opt_val = _no_delay ? 1 : 0;
  auto ret = ::setsockopt(socket_, SOL_SOCKET, TCP_NODELAY, (char*)&opt_val,
                          static_cast<socklen_t>(sizeof(opt_val)));
  return ret != 0 ? Error(ERROR_SOCKET_TCP_NO_DELAY) : Error();
}

auto Socket::SetReuseAddr(bool _reuse) const -> Error {
  auto optval = _reuse ? 1 : 0;
  auto ret = ::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (char*)&optval,
                          static_cast<socklen_t>(sizeof(optval)));
  return ret != 0 ? Error(ERROR_SOCKET_REUSE_ADDR) : Error();
}

auto Socket::SetReusePort(bool _reuse) const -> Error {
#ifdef SO_REUSEPORT
  auto opt_val = _reuse ? 1 : 0;
  auto ret = ::setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &opt_val,
                          static_cast<socklen_t>(sizeof(opt_val)));
#elif defined(H_OS_WIN)
  auto ret = 0;
  if (!reuse) {
    auto opt_val{1};
    ret = ::setsockopt(socket_, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                       (const char*)&opt_val,
                       static_cast<socklen_t>(sizeof(opt_val)));
  }
#else
  HARE_INTERNAL_ERROR("reuse-port is not supported.");
  auto ret = -1;
#endif
  return ret != 0 ? Error(ERROR_SOCKET_REUSE_PORT) : Error();
}

auto Socket::SetKeepAlive(bool _keep_alive) const -> Error {
  auto optval = _keep_alive ? 1 : 0;
  auto ret = ::setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval,
                          static_cast<socklen_t>(sizeof(optval)));
  return ret != 0 ? Error(ERROR_SOCKET_KEEP_ALIVE) : Error();
}

}  // namespace net
}  // namespace hare
