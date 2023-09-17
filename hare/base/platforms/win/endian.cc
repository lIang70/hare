#include <hare/base/io/operation.h>

#ifdef H_OS_WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#define socklen_t int
#define close closesocket

#if BYTE_ORDER == LITTLE_ENDIAN

#define htobe16(x) htons(x)
#define htole16(x) (x)
#define be16toh(x) ntohs(x)
#define le16toh(x) (x)

#define htobe32(x) htonl(x)
#define htole32(x) (x)
#define be32toh(x) ntohl(x)
#define le32toh(x) (x)

#define htobe64(x) htonll(x)
#define htole64(x) (x)
#define be64toh(x) ntohll(x)
#define le64toh(x) (x)

#elif BYTE_ORDER == BIG_ENDIAN

#define htobe16(x) (x)
#define htole16(x) __builtin_bswap16(x)
#define be16toh(x) (x)
#define le16toh(x) __builtin_bswap16(x)

#define htobe32(x) (x)
#define htole32(x) __builtin_bswap32(x)
#define be32toh(x) (x)
#define le32toh(x) __builtin_bswap32(x)

#define htobe64(x) (x)
#define htole64(x) __builtin_bswap64(x)
#define be64toh(x) (x)
#define le64toh(x) __builtin_bswap64(x)

#endif

namespace hare {
namespace io {

auto HostToNetwork64(std::uint64_t host64) -> std::uint64_t {
  return htobe64(host64);
}
auto HostToNetwork32(std::uint32_t host32) -> std::uint32_t {
  return htobe32(host32);
}
auto HostToNetwork16(std::uint16_t host16) -> std::uint16_t {
  return htobe16(host16);
}
auto NetworkToHost64(std::uint64_t net64) -> std::uint64_t {
  return be64toh(net64);
}
auto NetworkToHost32(std::uint32_t net32) -> std::uint32_t {
  return be32toh(net32);
}
auto NetworkToHost16(std::uint16_t net16) -> std::uint16_t {
  return be16toh(net16);
}

}  // namespace io
}  // namespace hare

#endif  // H_OS_LINUXs