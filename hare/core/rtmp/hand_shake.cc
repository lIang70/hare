#include "hare/core/protocol.h"
#include <hare/core/rtmp/hand_shake.h>

#include <hare/base/logging.h>
#include <hare/core/rtmp/protocol_rtmp.h>

#include <openssl/dh.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <cstdint>

#define RFC2409_PRIME_1024                             \
    "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1" \
    "29024E088A67CC74020BBEA63B139B22514A08798E3404DD" \
    "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245" \
    "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED" \
    "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381" \
    "FFFFFFFFFFFFFFFF"

// The digest key generate size.
#define OPENSSL_HASH_SIZE 512

// The rtmp c0c1 length
#define RTMP_C0C1_LENGTH 1537

namespace hare {
namespace core {

    namespace detail {

        // 68bytes FMS key which is used to sign the sever packet.
        static uint8_t genuine_fms_key[] = {
            0x47, 0x65, 0x6e, 0x75, 0x69, 0x6e, 0x65, 0x20,
            0x41, 0x64, 0x6f, 0x62, 0x65, 0x20, 0x46, 0x6c,
            0x61, 0x73, 0x68, 0x20, 0x4d, 0x65, 0x64, 0x69,
            0x61, 0x20, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72,
            0x20, 0x30, 0x30, 0x31, // Genuine Adobe Flash Media Server 001
            0xf0, 0xee, 0xc2, 0x4a, 0x80, 0x68, 0xbe, 0xe8,
            0x2e, 0x00, 0xd0, 0xd1, 0x02, 0x9e, 0x7e, 0x57,
            0x6e, 0xec, 0x5d, 0x2d, 0x29, 0x80, 0x6f, 0xab,
            0x93, 0xb8, 0xe6, 0x36, 0xcf, 0xeb, 0x31, 0xae
        }; // 68

        // 62bytes FP key which is used to sign the client packet.
        static uint8_t genuine_fp_key[] = {
            0x47, 0x65, 0x6E, 0x75, 0x69, 0x6E, 0x65, 0x20,
            0x41, 0x64, 0x6F, 0x62, 0x65, 0x20, 0x46, 0x6C,
            0x61, 0x73, 0x68, 0x20, 0x50, 0x6C, 0x61, 0x79,
            0x65, 0x72, 0x20, 0x30, 0x30, 0x31, // Genuine Adobe Flash Player 001
            0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8,
            0x2E, 0x00, 0xD0, 0xD1, 0x02, 0x9E, 0x7E, 0x57,
            0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB,
            0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
        }; // 62

        static auto opensslHMACSha256(const void* data, int data_size, const void* key, int key_size, void* digest) -> Error
        {
            unsigned int digest_size { 0 };

            if (key == nullptr) {
                // use data to digest.
                // @see ./crypto/sha/sha256t.c
                // @see ./crypto/evp/digest.c
                if (::EVP_Digest(data, data_size,
                        static_cast<unsigned char*>(digest), &digest_size,
                        ::EVP_sha256(), nullptr)
                    < 0) {
                    return Error(HARE_ERROR_OPENSSL_SHA256_EVP_DIGEST);
                }
            } else {
                ::HMAC(::EVP_sha256(),
                    static_cast<const unsigned char*>(key), key_size,
                    static_cast<const unsigned char*>(data), data_size,
                    static_cast<unsigned char*>(digest), &digest_size);

                if (digest_size != 32) {
                    return Error(HARE_ERROR_OPENSSL_SHA256_DIGEST_SIZE);
                }
            }
            return Error(HARE_ERROR_SUCCESS);
        }

    } // namespace detail

    HandShake::~HandShake()
    {
        delete[] c0c1_;
    }

    auto HandShake::readC0C1(net::Buffer& buffer) -> Error
    {
        if (c0c1_ != nullptr) { 
            return Error(HARE_ERROR_SUCCESS);
        }

        c0c1_ = new char[RTMP_C0C1_LENGTH];

        if (buffer.length() < RTMP_C0C1_LENGTH) {
            return Error(HARE_ERROR_RTMP_READ_C0C1);
        }

        auto read_size = buffer.remove(c0c1_, RTMP_C0C1_LENGTH);
        HARE_ASSERT(read_size == RTMP_C0C1_LENGTH, "read size unequal c0c1");

        // Whether RTMP proxy, @see https://github.com/ossrs/go-oryx/wiki/RtmpProxy
        if (static_cast<uint8_t>(c0c1_[0]) == static_cast<uint8_t>(RTMP_FMT_TYPE3)) {
            uint16_t csid = static_cast<uint16_t>(c0c1_[1])<<BITS_PER_BYTE | static_cast<uint16_t>(c0c1_[2]);
            ssize_t csid_consumed = 3 + csid;
            if (csid > ONE_KILO) {
                return Error(HARE_ERROR_RTMP_PROXY_EXCEED);
            }

            // 4B client real IP.
            if (csid >= 4) {
                proxy_real_ip_ = 0;
                for (auto index = 0; index <= 3; ++index) {
                    proxy_real_ip_ |= (static_cast<uint32_t>(c0c1_[3 + index])<<(BITS_PER_BYTE * (3 - index)));
                }
                csid -= 4;
            }

            memmove(c0c1_, c0c1_ + csid_consumed, RTMP_C0C1_LENGTH - csid_consumed);
            if (buffer.length() < RTMP_C0C1_LENGTH) {
                type_ = RTMP_FMT_TYPE3;
                return Error(HARE_ERROR_RTMP_READ_C0C1);
            }
            // if ((err = io->read_fully(c0c1_ + RTMP_C0C1_LENGTH - csid_consumed, csid_consumed, &nsize)) != srs_success) {
            //     return srs_error_wrap(err, "read c0c1");
            // }
        }

        return Error(HARE_ERROR_SUCCESS);
    }

    auto CpxHandShake::handShakeWithClient(net::Buffer& buffer, StreamSession::Ptr session) -> Error
    {
        auto err = readC0C1(buffer);
        if (!err) {
            return err;
        }

        return Error(HARE_ERROR_SUCCESS);
    }

    auto CpxHandShake::handShakeWithServer(net::Buffer& buffer, StreamSession::Ptr session) -> Error
    {
        return Error(HARE_ERROR_SUCCESS);
    }

} // namespace core
} // namespace hare