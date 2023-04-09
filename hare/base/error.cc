#include <hare/base/error.h>

#include <array>
#include <cerrno>

namespace hare {

namespace detail {

    static const auto description_length { HARE_ERRORS };
    static std::array<const char*, description_length> error_description {
        "Illegal error occurred, please contact the developer.",    // HARE_ERROR_ILLEGAL
        "Success.",                                                 // HARE_ERROR_SUCCESS
        
        // About thread
        "The thread is already running.",                           // HARE_ERROR_THREAD_ALREADY_RUNNING
        "The task of thread is empty.",                             // HARE_ERROR_THREAD_TASK_EMPTY
        "Failed to create a thread.",                               // HARE_ERROR_FAILED_CREATE_THREAD
        "Cannot join in the same thread.",                          // HARE_ERROR_JOIN_SAME_THREAD
        "The thread has exited.",                                   // HARE_ERROR_THREAD_EXITED
        "The thread-pool is already running.",                      // HARE_ERROR_TPOOL_ALREADY_RUNNING
        "The thread-pool is not running.",                          // HARE_ERROR_TPOOL_NOT_RUNNING
        "The thread number of thread-pool is zero.",                // HARE_ERROR_TPOOL_THREAD_ZERO
        
        // Net
        "Cannot get local address.",                                // HARE_ERROR_GET_LOCAL_ADDRESS
        "The family of address is [AF_INET6/AF_INET] .",            // HARE_ERROR_WRONG_FAMILY

        // OpenSSL
        "Failed when openssl create the dh.",                       // HARE_ERROR_OPENSSL_CREATE_DH
        "Failed when openssl create the private key.",              // HARE_ERROR_OPENSSL_CREATE_PRIVATE_KEY
        "Failed when openssl create G.",                            // HARE_ERROR_OPENSSL_CREATE_G
        "Failed when openssl set G.",                               // HARE_ERROR_OPENSSL_SET_G,
        "Failed when openssl parse P1024.",                         // HARE_ERROR_OPENSSL_PARSE_P1024,
        "Failed when openssl generate DHKeys.",                     // HARE_ERROR_OPENSSL_GENERATE_DH_KEYS,
        "Failed when openssl share key already computed.",          // HARE_ERROR_OPENSSL_SHARE_KEY_COMPUTED,
        "Failed when openssl get shared key size.",                 // HARE_ERROR_OPENSSL_GET_SHARED_KEY_SIZE,
        "Failed when openssl get peer public key.",                 // HARE_ERROR_OPENSSL_GET_PEER_PUBLIC_KEY,
        "Failed when openssl compute shared key.",                  // HARE_ERROR_OPENSSL_COMPUTE_SHARED_KEY,
        "Failed when openssl is invalid DH state.",                 // HARE_ERROR_OPENSSL_INVALID_DH_STATE,
        "Failed when openssl copy key.",                            // HARE_ERROR_OPENSSL_COPY_KEY,
        "Failed to calculate evp digest of HMAC sha256 for SSL.",   // HARE_ERROR_OPENSSL_SHA256_EVP_DIGEST,
        "Failed when openssl sha256 digest key invalid size.",      // HARE_ERROR_OPENSSL_SHA256_DIGEST_SIZE,

        // RTMP
        "", // HARE_ERROR_RTMP_READ_C0C1
        "", // HARE_ERROR_RTMP_PROXY_EXCEED
    };

} // namespace detail

Error::Error(int32_t error_code)
    : error_code_(error_code)
    , system_code_(errno)
{
}

auto Error::description() const -> const char*
{
    return detail::error_description[error_code_];
}

} // namespace hare