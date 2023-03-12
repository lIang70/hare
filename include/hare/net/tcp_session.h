#ifndef _HARE_NET_TCP_SESSION_H_
#define _HARE_NET_TCP_SESSION_H_

#include <hare/base/detail/non_copyable.h>
#include <hare/base/util.h>
#include <hare/net/host_address.h>
#include <hare/base/timestamp.h>
#include <hare/net/buffer.h>

#include <memory>

namespace hare {
namespace core {
    class Cycle;
}

namespace net {

    class TcpSessionPrivate;
    class HARE_API TcpSession : public NonCopyable
                              , public std::enable_shared_from_this<TcpSession> {
        friend class TcpServe;
        friend class TcpServePrivate;
        friend class TcpSessionPrivate;

        TcpSessionPrivate* p_ { nullptr };

    public:
        enum State : int8_t {
            CONNECTING = 0x00,
            CONNECTED,
            DISCONNECTING
        };

        virtual ~TcpSession();

        const std::string& name() const;
        const HostAddress& localAddress() const;
        const HostAddress& peerAddress() const;

        void setHighWaterMark(std::size_t high_water = 64 * 1024 * 1024);

        State state() const;

        util_socket_t socket() const;

        bool send(const uint8_t* bytes, std::size_t length);

        void shutdown(); // NOT thread safe, no simultaneous calling
        void forceClose();
        void setTcpNoDelay(bool on);

        // reading or not
        void startRead();
        void stopRead();

    protected:
        virtual void connection(int32_t flag) {}
        virtual void writeComplete() {}
        virtual void highWaterMark() {}
        virtual void read(Buffer& b, const Timestamp& ts) {}

    private:
        explicit TcpSession(TcpSessionPrivate* p);

        core::Cycle* getCycle();

        void shutdownInCycle();
        void forceCloseInCycle();
        void startReadInCycle();
        void stopReadInCycle();
        void writeInCycle();

        void connectEstablished();
        void connectDestroyed();
    };

    using STcpSession = std::shared_ptr<TcpSession>;
    using WTcpSession = std::weak_ptr<TcpSession>;

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SESSION_H_