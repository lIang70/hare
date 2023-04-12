#ifndef _HARE_NET_SESSION_H_
#define _HARE_NET_SESSION_H_

#include <hare/base/time/timestamp.h>
#include <hare/net/buffer.h>
#include <hare/net/core/event.h>
#include <hare/net/host_address.h>
#include <hare/net/socket.h>

namespace hare {
namespace net {

    class HARE_API Session : public NonCopyable {
    public:
        using Ptr = std::shared_ptr<Session>;

        using STATE = enum State : int8_t {
            STATE_CONNECTING = 0x00,
            STATE_CONNECTED,
            STATE_DISCONNECTING,
            STATE_DISCONNECTED
        };

        static const std::size_t DEFAULT_HIGH_WATER = static_cast<const std::size_t>(64 * 1024 * 1024);

    private:
        const std::string name_ {};

    protected:
        core::Cycle* cycle_ { nullptr };
        Socket::Ptr socket_ { nullptr };
        core::Event::Ptr event_ { nullptr };

        const HostAddress local_addr_ {};
        const HostAddress peer_addr_ {};

        Buffer in_buffer_ {};
        Buffer out_buffer_ {};
        std::size_t high_water_mark_ { DEFAULT_HIGH_WATER };

        bool reading_ { false };
        State state_ { STATE_CONNECTING };

    public:
        virtual ~Session();

        inline auto name() const -> std::string { return name_; }
        inline auto localAddress() const -> const HostAddress& { return local_addr_; }
        inline auto peerAddress() const -> const HostAddress& { return local_addr_; }
        inline auto state() const -> State { return state_; }
        inline auto socket() const -> util_socket_t { return socket_->socket(); }

        inline void setHighWaterMark(std::size_t high_water = DEFAULT_HIGH_WATER) { high_water_mark_ = high_water; }

        virtual auto send(const uint8_t* bytes, std::size_t length) -> bool = 0;

        auto shutdown() -> Error;
        auto forceClose() -> Error;

        void startRead();
        void stopRead();

    protected:
        Session(core::Cycle* cycle, Socket::TYPE type, 
            HostAddress local_addr,
            std::string name, int8_t family, util_socket_t target_fd,
            HostAddress peer_addr);

        inline void setState(STATE state) { state_ = state; }

        virtual void connection(int32_t flag) = 0;
        virtual void writeComplete() = 0;
        virtual void highWaterMark() = 0;
        virtual void read(Buffer& buffer, const Timestamp& time) = 0;
        
        virtual void handleRead(const Timestamp& receive_time) = 0;
        virtual void handleWrite() = 0;
        void handleClose();
        void handleError();

    private:
        void connectEstablished();
        void connectDestroyed();

        friend class HybridServe;
    };

} // namespace net
} // namespace hare

#endif // ! _HARE_NET_SESSION_H_