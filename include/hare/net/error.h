/**
 * @file hare/net/error.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with error.h
 * @version 0.1-beta
 * @date 2023-07-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_ERROR_H_
#define _HARE_NET_ERROR_H_

#include <hare/base/fwd.h>

namespace hare {
namespace net {

    using ERROR = enum : std::int32_t {
        ERROR_ILLEGAL = -1,
        ERROR_SUCCESS,

        ERROR_GET_LOCAL_ADDRESS,
        ERROR_WRONG_FAMILY,
        ERROR_SOCKET_BIND,
        ERROR_SOCKET_LISTEN,
        ERROR_SOCKET_CLOSED,
        ERROR_SOCKET_CONNECT,
        ERROR_SOCKET_SHUTDOWN_WRITE,
        ERROR_SOCKET_FROM_IP,
        ERROR_SOCKET_TCP_NO_DELAY,
        ERROR_SOCKET_REUSE_ADDR,
        ERROR_SOCKET_REUSE_PORT,
        ERROR_SOCKET_KEEP_ALIVE,
        ERROR_SOCKET_WRITING,
        ERROR_ACCEPTOR_ACTIVED,
        ERROR_SESSION_ALREADY_DISCONNECT,
        ERROR_GET_SOCKET_PAIR,
        ERROR_INIT_IO_POOL,
        
        ERRORS_NBR
    };

    HARE_CLASS_API
    class HARE_API error {
        std::int32_t error_code_ { ERROR_SUCCESS };
        std::int32_t system_code_ { 0 };

    public:
        explicit error(std::int32_t error_code = ERROR_SUCCESS);
        virtual ~error() = default;

        /**
         * @brief Get the error code of hare.
         *
         **/
        HARE_INLINE auto code() const -> std::int32_t { return error_code_; }

        /**
         * @brief Get the system code 'errno'.
         *
         **/
        HARE_INLINE auto system_code() const -> std::int32_t { return system_code_; }

        HARE_INLINE explicit operator bool() const { return error_code_ == ERROR_SUCCESS; }
        HARE_INLINE auto operator==(ERROR error) const -> bool { return error_code_ == error; }

        /**
         * @brief Get the description of the error code.
         *
         **/
        virtual auto description() const -> const char*;
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_ERROR_H_