#ifndef _HARE_NET_TCP_SESSION_H_
#define _HARE_NET_TCP_SESSION_H_

#include <hare/base/time/timestamp.h>
#include <hare/base/util/non_copyable.h>
#include <hare/net/buffer.h>
#include <hare/net/host_address.h>

namespace hare {
namespace core {
    class Cycle;
}

namespace net {

    class TcpSessionPrivate;
    class HARE_API TcpSession : public NonCopyable, public std::enable_shared_from_this<TcpSession> {
        friend class TcpServe;
        friend class TcpServePrivate;
        friend class TcpSessionPrivate;

        TcpSessionPrivate* p_ { nullptr };

    public:
        using Ptr = std::shared_ptr<TcpSession>;

        enum State : int8_t {
            CONNECTING = 0x00,
            CONNECTED,
            DISCONNECTING
        };

        static const std::size_t DEFAULT_HIGH_WATER = static_cast<const std::size_t>(64 * 1024 * 1024);

        virtual ~TcpSession();

        auto name() const -> const std::string&;
        auto localAddress() const -> const HostAddress&;
        auto peerAddress() const -> const HostAddress&;

        void setHighWaterMark(std::size_t high_water = DEFAULT_HIGH_WATER);

        auto state() const -> State;

        auto socket() const -> util_socket_t;

        auto send(const uint8_t* bytes, std::size_t length) -> bool;

        void shutdown(); // NOT thread safe, no simultaneous calling
        void forceClose();
        void setTcpNoDelay(bool tcp_on);

        // reading or not
        void startRead();
        void stopRead();

    protected:
        explicit TcpSession(TcpSessionPrivate* tsp);

        virtual void connection(int32_t flag) { }
        virtual void writeComplete() { }
        virtual void highWaterMark() { }
        virtual void read(Buffer& buffer, const Timestamp& time) { }

        void shutdownInCycle();
        void forceCloseInCycle();
        void startReadInCycle();
        void stopReadInCycle();
        void writeInCycle();

    private:
        auto getCycle() -> core::Cycle*;

        void connectEstablished();
        void connectDestroyed();
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SESSION_H_