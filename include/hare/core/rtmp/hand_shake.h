#ifndef _HARE_CORE_HAND_SHAKE_H_
#define _HARE_CORE_HAND_SHAKE_H_

#include <hare/base/error.h>
#include <hare/net/buffer.h>
#include <hare/core/stream_session.h>
#include <sys/types.h>

namespace hare {
namespace core {

    class HandShake {

        uint32_t proxy_real_ip_ { 0 };
        int32_t type_ { -1 };
        char* c0c1_ { nullptr };

    public:
        using Ptr = std::shared_ptr<HandShake>;

        HandShake() = default;
        virtual ~HandShake();

        virtual auto handShakeWithClient(net::Buffer& buffer, StreamSession::Ptr session) -> Error = 0;
        virtual auto handShakeWithServer(net::Buffer& buffer, StreamSession::Ptr session) -> Error = 0;

    protected:
        auto readC0C1(net::Buffer& buffer) -> Error;

    };

    class CpxHandShake final : public HandShake {
    public:
        ~CpxHandShake() final = default;

        auto handShakeWithClient(net::Buffer& buffer, StreamSession::Ptr session) -> Error final;
        auto handShakeWithServer(net::Buffer& buffer, StreamSession::Ptr session) -> Error final;
    };

    class SimpleHandShake final : public HandShake {
    public:
        ~SimpleHandShake() final = default;

        auto handShakeWithClient(net::Buffer& buffer, StreamSession::Ptr session) -> Error final;
        auto handShakeWithServer(net::Buffer& buffer, StreamSession::Ptr session) -> Error final;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_HAND_SHAKE_H_