#include <hare/base/error.h>

#include <array>
#include <cerrno>

namespace hare {

namespace detail {

    static const auto description_length { HARE_ERRORS };
    static std::array<const char*, description_length> error_description {
        "Illegal error occurred, please contact the developer.",    // HARE_ERROR_ILLEGAL
        "Success.",                                                 // HARE_ERROR_SUCCESS
        
        // Base
        "Cannot set name of thread.",                               // HARE_ERROR_SET_THREAD_NAME

        // Net
        "Cannot get local address.",                                // HARE_ERROR_GET_LOCAL_ADDRESS
        "The family of address is [AF_INET6/AF_INET] .",            // HARE_ERROR_WRONG_FAMILY
        "Failed to bind socket.",                                   // HARE_ERROR_SOCKET_BIND
        "Failed to listen at specified port  .",                    // HARE_ERROR_SOCKET_LISTEN
        "Failed to close socket.",                                  // HARE_ERROR_SOCKET_CLOSED
        "Failed to connect socket.",                                // HARE_ERROR_SOCKET_CONNECT
        "Failed to shutdown write of socket.",                      // HARE_ERROR_SOCKET_SHUTDOWN_WRITE
        "Failed to get addr from ip/port.",                         // HARE_ERROR_SOCKET_FROM_IP
        "Failed to set no-delay to tcp socket.",                    // HARE_ERROR_SOCKET_TCP_NO_DELAY
        "Failed to set reuse address to socket.",                   // HARE_ERROR_SOCKET_REUSE_ADDR
        "Failed to set reuse port to socket.",                      // HARE_ERROR_SOCKET_REUSE_PORT
        "Failed to set keep alive to socket.",                      // HARE_ERROR_SOCKET_KEEP_ALIVE
        "Failed to shutdown, because socket is writing.",           // HARE_ERROR_SOCKET_WRITING
        "Failed to active acceptor.",                               // HARE_ERROR_ACCEPTOR_ACTIVED
        "Session already disconnected.",                            // HARE_ERROR_SESSION_ALREADY_DISCONNECT
        "Failed to get pair socket.",                               // HARE_ERROR_GET_SOCKET_PAIR

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

error::error(int32_t error_code)
    : error_code_(error_code)
    , system_code_(errno)
{
}

auto error::description() const -> const char*
{
    return detail::error_description[error_code_];
}

} // namespace hare