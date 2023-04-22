#ifndef _HARE_STREAMING_HAND_SHAKE_RTMP_H_
#define _HARE_STREAMING_HAND_SHAKE_RTMP_H_

#include <hare/base/error.h>
#include <hare/base/io/buffer.h>

namespace hare {
namespace net {
    class session;
} // namespace net

namespace streaming {

    class handshake {
        uint32_t proxy_real_ip_ { 0 };
        int32_t type_ { -1 };
        char* c0c1_ { nullptr };

    public:
        using ptr = ptr<handshake>;

        handshake() = default;
        virtual ~handshake();

        virtual auto hand_shake_client(io::buffer& buffer, hare::ptr<net::session> session) -> error = 0;
        virtual auto hand_shake_server(io::buffer& buffer, hare::ptr<net::session> session) -> error = 0;

    protected:
        auto read_c0c1(io::buffer& buffer) -> error;

    };

    class complex_handshake final : public handshake {
    public:
        ~complex_handshake() final = default;

        auto hand_shake_client(io::buffer& buffer, hare::ptr<net::session> session) -> error final;
        auto hand_shake_server(io::buffer& buffer, hare::ptr<net::session> session) -> error final;
    };

    class simple_handshake final : public handshake {
    public:
        ~simple_handshake() final = default;

        auto hand_shake_client(io::buffer& buffer, hare::ptr<net::session> session) -> error final;
        auto hand_shake_server(io::buffer& buffer, hare::ptr<net::session> session) -> error final;
    };

} // namespace streaming
} // namespace hare

#endif // !_HARE_STREAMING_HAND_SHAKE_RTMP_H_