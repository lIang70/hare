#include <hare/base/io/operation.h>

#ifdef H_OS_LINUX

#include <endian.h>

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