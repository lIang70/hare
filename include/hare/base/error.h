/**
 * @file hare/base/error.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with error.h
 * @version 0.1-beta
 * @date 2023-04-05
 * 
 * @copyright Copyright (c) 2023
 * 
 **/

#ifndef _HARE_BASE_ERROR_H_
#define _HARE_BASE_ERROR_H_

#include <hare/base/util.h>

namespace hare {

using HARE_ERROR = enum {
    HARE_ERROR_ILLEGAL,
    HARE_ERROR_SUCCESS,

    // Base
    HARE_ERROR_SET_THREAD_NAME,

    // Net
    HARE_ERROR_GET_LOCAL_ADDRESS,
    HARE_ERROR_WRONG_FAMILY,
    HARE_ERROR_SOCKET_BIND,
    HARE_ERROR_SOCKET_LISTEN,
    HARE_ERROR_SOCKET_CLOSED,
    HARE_ERROR_SOCKET_CONNECT,
    HARE_ERROR_SOCKET_SHUTDOWN_WRITE,
    HARE_ERROR_SOCKET_FROM_IP,
    HARE_ERROR_SOCKET_TCP_NO_DELAY,
    HARE_ERROR_SOCKET_REUSE_ADDR,
    HARE_ERROR_SOCKET_REUSE_PORT,
    HARE_ERROR_SOCKET_KEEP_ALIVE,
    HARE_ERROR_SOCKET_WRITING,
    HARE_ERROR_ACCEPTOR_ACTIVED,
    HARE_ERROR_SESSION_ALREADY_DISCONNECT,
    HARE_ERROR_GET_SOCKET_PAIR,
    HARE_ERROR_INIT_IO_POOL,

    // OpenSSL
    HARE_ERROR_OPENSSL_CREATE_DH,
    HARE_ERROR_OPENSSL_CREATE_PRIVATE_KEY,
    HARE_ERROR_OPENSSL_CREATE_G,
    HARE_ERROR_OPENSSL_SET_G,
    HARE_ERROR_OPENSSL_PARSE_P1024,
    HARE_ERROR_OPENSSL_GENERATE_DH_KEYS,
    HARE_ERROR_OPENSSL_SHARE_KEY_COMPUTED,
    HARE_ERROR_OPENSSL_GET_SHARED_KEY_SIZE,
    HARE_ERROR_OPENSSL_GET_PEER_PUBLIC_KEY,
    HARE_ERROR_OPENSSL_COMPUTE_SHARED_KEY,
    HARE_ERROR_OPENSSL_INVALID_DH_STATE,
    HARE_ERROR_OPENSSL_COPY_KEY,
    HARE_ERROR_OPENSSL_SHA256_EVP_DIGEST,
    HARE_ERROR_OPENSSL_SHA256_DIGEST_SIZE,

    // RTMP
    HARE_ERROR_RTMP_READ_C0C1,
    HARE_ERROR_RTMP_PROXY_EXCEED,

    HARE_ERRORS
}; 

class HARE_API error {
    int32_t error_code_ { HARE_ERROR_SUCCESS };
    int32_t system_code_ { 0 };

public:
    explicit error(int32_t error_code);
    ~error() = default;

    /**
     * @brief Get the error code of hare.
     * 
     **/
    inline auto code() const -> int32_t { return error_code_; }
    
    /**
     * @brief Get the system code 'errno'.
     * 
     **/
    inline auto system_code() const -> int32_t { return system_code_; }

    /**
     * @brief Get the description of the error code.
     * 
     **/
    auto description() const -> const char*;

    inline explicit operator bool() const { return error_code_ == HARE_ERROR_SUCCESS; }
    inline auto operator==(HARE_ERROR error) const -> bool { return error_code_ == error; }
};

} // namespace hare

#endif // !_HARE_BASE_ERROR_H_