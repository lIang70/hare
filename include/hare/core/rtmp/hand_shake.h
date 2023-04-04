#ifndef _HARE_CORE_HAND_SHAKE_H_
#define _HARE_CORE_HAND_SHAKE_H_

#include <hare/base/util/util.h>
#include <hare/net/buffer.h>

namespace hare {
namespace core {

    class CpxHandShake {
    public:
        using Ptr = std::shared_ptr<CpxHandShake>;

        enum Error : int32_t {
            NONE = 0,

            // failed when open ssl create the dh
            OPENSSL_CREATE_DH,
            // failed when open ssl create the private key.
            OPENSSL_CREATE_PRIVATE_KEY,
            // when open ssl create G.
            OPENSSL_CREATE_G,
            // when open ssl set G
            OPENSSL_SET_G,
            // when open ssl parse P1024
            OPENSSL_PARSE_P1024,
            // when open ssl generate DHKeys
            OPENSSL_GENERATE_DH_KEYS,
            // when open ssl share key already computed.
            OPENSSL_SHARE_KEY_COMPUTED,
            // when open ssl get shared key size
            OPENSSL_GET_SHARED_KEY_SIZE,
            // when open ssl get peer public key.
            OPENSSL_GET_PEER_PUBLIC_KEY,
            // when open ssl compute shared key.
            OPENSSL_COMPUTE_SHARED_KEY,
            // when open ssl is invalid DH state.
            OPENSSL_INVALID_DH_STATE,
            // when open ssl copy key
            OPENSSL_COPY_KEY,
            // failed to calculate evp digest of HMAC sha256 for SSL
            OPENSSL_SHA256_EVP_DIGEST,
            // when open ssl sha256 digest key invalid size.
            OPENSSL_SHA256_DIGEST_SIZE,
        };

        CpxHandShake() = default;
        virtual ~CpxHandShake() = default;

        virtual auto handShake(net::Buffer& buffer, const char* c0c1) -> Error;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_HAND_SHAKE_H_