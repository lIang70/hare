/**
 * @file hare/base/io/socket_op.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with socket_op.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_IO_SOCKET_OP_H_
#define _HARE_BASE_IO_SOCKET_OP_H_

#include <hare/base/fwd.h>

namespace hare {
namespace socket_op {

    HARE_API auto host_to_network64(std::uint64_t _host64) -> std::uint64_t;
    HARE_API auto host_to_network32(std::uint32_t _host32) -> std::uint32_t;
    HARE_API auto host_to_network16(std::uint16_t _host16) -> std::uint16_t;
    HARE_API auto network_to_host64(std::uint64_t _net64) -> std::uint64_t;
    HARE_API auto network_to_host32(std::uint32_t _net32) -> std::uint32_t;
    HARE_API auto network_to_host16(std::uint16_t _net16) -> std::uint16_t;

    HARE_API auto get_socket_error_info(util_socket_t _fd) -> std::string;
    HARE_API auto socketpair(std::uint8_t _family, std::int32_t _type, std::int32_t _protocol, util_socket_t _socket[2]) -> std::int32_t;

} // namespace socket_op
} // namespace hare

#endif // _HARE_BASE_IO_SOCKET_OP_H_