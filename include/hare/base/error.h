/**
 * @file hare/base/error.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with error.h
 * @version 0.1-beta
 * @date 2023-04-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _HARE_BASE_ERROR_H_
#define _HARE_BASE_ERROR_H_

#include <hare/base/util.h>

namespace hare {

using HARE_ERROR = enum Hare_Error {
    HARE_ERROR_ILLEGAL,
    HARE_ERROR_SUCCESS,

    // About thread
    HARE_ERROR_THREAD_ALREADY_RUNNING,
    HARE_ERROR_THREAD_TASK_EMPTY,
    HARE_ERROR_FAILED_CREATE_THREAD,
    HARE_ERROR_JOIN_SAME_THREAD,
    HARE_ERROR_THREAD_EXITED,
    HARE_ERROR_TPOOL_ALREADY_RUNNING,
    HARE_ERROR_TPOOL_NOT_RUNNING,
    HARE_ERROR_TPOOL_THREAD_ZERO,

    // Net
    HARE_ERROR_GET_LOCAL_ADDRESS,
    HARE_ERROR_WRONG_FAMILY,

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

class HARE_API Error {
    int32_t error_code_ { HARE_ERROR_SUCCESS };
    int32_t system_code_ { 0 };

public:
    explicit Error(int32_t error_code);
    ~Error() = default;

    /**
     * @brief Get the error code of hare.
     * 
     */
    inline auto code() const -> int32_t { return error_code_; }
    
    /**
     * @brief Get the system code 'errno'.
     * 
     */
    inline auto systemCode() const -> int32_t { return system_code_; }

    /**
     * @brief Get the description of the error code.
     * 
     */
    auto description() const -> const char*;

    inline explicit operator bool() const { return error_code_ == HARE_ERROR_SUCCESS; }
};

} // namespace hare

#endif // !_HARE_BASE_ERROR_H_