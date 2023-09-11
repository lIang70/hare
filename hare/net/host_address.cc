#include <hare/hare-config.h>
#include <hare/net/host_address.h>

#include <cstring>

#include "base/fwd-inl.h"
#include "socket_op.h"

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif  // HARE__HAVE_NETINET_IN_H

#if HARE__HAVE_NETDB_H
#include <netdb.h>
#endif  // HARE__HAVE_NETDB_H

#if defined(H_OS_WIN)
#include <WinSock2.h>
#include <Ws2tcpip.h>
#define socklen_t int
#endif

namespace hare {
namespace net {

static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

auto HostAddress::Resolve(const std::string& _hostname, HostAddress* _result)
    -> bool {
  HARE_ASSERT(_result != nullptr);
  HARE_ASSERT(_result->Family() == AF_INET);

  addrinfo hints{}, *res{};
  hare::detail::FillN(&hints, sizeof(addrinfo), 0);
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = _result->Family();
  auto ret{0};

  if ((ret = ::getaddrinfo(_hostname.c_str(), nullptr, &hints, &res)) != 0) {
    HARE_INTERNAL_ERROR("::getaddrinfo error {} : {}.", ret,
                        ::gai_strerror(ret));
    return false;
  }
  socket_op::SockaddrCastIn(_result->in_)->sin_addr =
      socket_op::SockaddrCastIn(res->ai_addr)->sin_addr;
  return true;
}

auto HostAddress::LocalAddress(util_socket_t _fd) -> HostAddress {
  HostAddress local_addr{};
  local_addr.in_ =
      static_cast<struct sockaddr*>(std::malloc(sizeof(struct sockaddr_in6)));

  auto addr_len = static_cast<socklen_t>(sizeof(struct sockaddr_in6));
  if (::getsockname(_fd, local_addr.in_, &addr_len) < 0) {
    HARE_INTERNAL_ERROR("cannot get local addr.");
  }
  return local_addr;
}

auto HostAddress::PeerAddress(util_socket_t _fd) -> HostAddress {
  HostAddress peer_addr{};
  peer_addr.in_ =
      static_cast<struct sockaddr*>(std::malloc(sizeof(struct sockaddr_in6)));

  auto addr_len = static_cast<socklen_t>(sizeof(struct sockaddr_in6));
  if (::getpeername(_fd, peer_addr.in_, &addr_len) < 0) {
    HARE_INTERNAL_ERROR("cannot get peer addr.");
  }
  return peer_addr;
}

HostAddress::HostAddress(std::uint16_t _port, bool _loopback_only, bool _ipv6)
    : in_(static_cast<struct sockaddr*>(
          std::malloc(sizeof(struct sockaddr_in6)))) {
  hare::detail::FillN(in_, sizeof(struct sockaddr_in6), 0);

  if (_ipv6) {
    auto* in6 = socket_op::SockaddrCastIn6(in_);
    in6->sin6_family = AF_INET6;
    auto addr = _loopback_only ? in6addr_loopback : in6addr_any;
    in6->sin6_addr = addr;
    in6->sin6_port = io::HostToNetwork16(_port);
  } else {
    auto* in = socket_op::SockaddrCastIn(in_);
    in->sin_family = AF_INET;
    auto addr = _loopback_only ? INADDR_LOOPBACK : INADDR_ANY;
    in->sin_addr.s_addr = addr;
    in->sin_port = io::HostToNetwork16(_port);
  }
}

HostAddress::HostAddress(const std::string& _ip, std::uint16_t _port,
                         bool _ipv6)
    : in_(static_cast<struct sockaddr*>(
          std::malloc(sizeof(struct sockaddr_in6)))) {
  hare::detail::FillN(in_, sizeof(struct sockaddr_in6), 0);

  if (_ipv6 || (strchr(_ip.c_str(), ':') != nullptr)) {
    socket_op::FromIpPort(_ip.c_str(), _port, socket_op::SockaddrCastIn6(in_));
  } else {
    socket_op::FromIpPort(_ip.c_str(), _port, socket_op::SockaddrCastIn(in_));
  }
}

HostAddress::~HostAddress() { std::free(in_); }

HostAddress::HostAddress(HostAddress&& _another) noexcept {
  std::swap(in_, _another.in_);
}

auto HostAddress::operator=(HostAddress&& _another) noexcept -> HostAddress& {
  std::swap(in_, _another.in_);
  std::free(_another.in_);
  return (*this);
}

auto HostAddress::Clone() const -> hare::Ptr<HostAddress> {
  auto clone = std::make_shared<HostAddress>();
  clone->in_ =
      static_cast<struct sockaddr*>(std::malloc(sizeof(struct sockaddr_in6)));
  hare::detail::FillN((clone->in_), sizeof(struct sockaddr_in6), 0);

  if (Family() == AF_INET6) {
    auto* in6 = socket_op::SockaddrCastIn6(in_);
    auto* clone_in6 = socket_op::SockaddrCastIn6(clone->in_);
    clone_in6->sin6_family = AF_INET6;
    clone_in6->sin6_addr = in6->sin6_addr;
    clone_in6->sin6_port = in6->sin6_port;
  } else {
    auto* in = socket_op::SockaddrCastIn(in_);
    auto* clone_in = socket_op::SockaddrCastIn(clone->in_);
    clone_in->sin_family = AF_INET;
    clone_in->sin_addr.s_addr = in->sin_addr.s_addr;
    clone_in->sin_port = in->sin_port;
  }
  return clone;
}

auto HostAddress::Family() const -> std::uint8_t {
  return socket_op::SockaddrCastIn(in_)->sin_family;
}

auto HostAddress::ToIp() const -> std::string {
  std::array<char, HARE_SMALL_FIXED_SIZE * 2> cache{};
  socket_op::ToIp(cache.data(), HARE_SMALL_FIXED_SIZE * 2, in_);
  return cache.data();
}

auto HostAddress::ToIpPort() const -> std::string {
  std::array<char, HARE_SMALL_FIXED_SIZE * 2> cache{};
  socket_op::ToIpPort(cache.data(), HARE_SMALL_FIXED_SIZE * 2, in_);
  return cache.data();
}

auto HostAddress::Ipv4NetEndian() const -> std::uint32_t {
  HARE_ASSERT(socket_op::SockaddrCastIn(in_)->sin_family == AF_INET);
  return socket_op::SockaddrCastIn(in_)->sin_addr.s_addr;
}

auto HostAddress::PortNetEndian() const -> std::uint16_t {
  return socket_op::SockaddrCastIn(in_)->sin_port;
}

// set IPv6 ScopeID
void HostAddress::SetScopeId(std::uint32_t _id) const {
  if (socket_op::SockaddrCastIn(in_)->sin_family == AF_INET6) {
    socket_op::SockaddrCastIn6(in_)->sin6_scope_id = _id;
  }
}

}  // namespace net
}  // namespace hare
