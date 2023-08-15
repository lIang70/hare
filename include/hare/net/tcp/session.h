/**
 * @file hare/net/tcp/session.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with session.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_TCP_SESSION_H_
#define _HARE_NET_TCP_SESSION_H_

#include <hare/base/io/event.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/util/any.h>
#include <hare/net/buffer.h>
#include <hare/net/socket.h>

namespace hare {
namespace net {

    using SessionEvent = enum : std::uint8_t {
        SESSION_DEFAULT = io::EVENT_DEFAULT,
        SESSION_READ = io::EVENT_READ,
        SESSION_WRITE = io::EVENT_WRITE,
        SESSION_CLOSED = io::EVENT_CLOSED,
        SESSION_ERROR = 0x40,
        SESSION_CONNECTED = 0x80,
    };

    using SessionState = enum : std::int8_t {
        STATE_CONNECTING = 0x00,
        STATE_CONNECTED,
        STATE_DISCONNECTING,
        STATE_DISCONNECTED
    };

    HARE_CLASS_API
    class HARE_API TcpSession : public util::NonCopyable
                              , public std::enable_shared_from_this<TcpSession> {
        hare::detail::Impl* impl_ {};

    public:
        using ConnectCallback = std::function<void(const hare::Ptr<TcpSession>&, std::uint8_t)>;
        using WriteCallback = std::function<void(const hare::Ptr<TcpSession>&)>;
        using HighWaterCallback = std::function<void(const hare::Ptr<TcpSession>&)>;
        using ReadRallback = std::function<void(const hare::Ptr<TcpSession>&, Buffer&, const Timestamp&)>;
        using SessionDestroy = std::function<void()>;

        virtual ~TcpSession();

        auto Name() const -> std::string;
        auto OwnerCycle() const -> io::Cycle*;
        auto LocalAddress() const -> const HostAddress&;
        auto PeerAddress() const -> const HostAddress&;
        auto State() const -> SessionState;
        auto Fd() const -> util_socket_t;

        void SetConnectCallback(ConnectCallback _connect);
        void SetReadCallback(ReadRallback _read);
        void SetWriteCallback(WriteCallback _write);
        void SetHighWaterCallback(HighWaterCallback _high_water);
        void SetHighWaterMark(std::size_t _hwm);

        void SetContext(const util::Any& context);
        auto GetContext() const -> const util::Any&;

        auto Shutdown() -> Error;
        // not thread-safe
        auto ForceClose() -> Error;

        void StartRead();
        void StopRead();

        HARE_INLINE auto Connected() const -> bool { return State() == STATE_CONNECTED; }

        auto Append(Buffer& _buffer) -> bool;
        auto Send(const void* _bytes, std::size_t _length) -> bool;

        HARE_INLINE auto SetTcpNoDelay(bool _on) -> Error { return Socket().SetTcpNoDelay(_on); }

    protected:
        TcpSession(io::Cycle* _cycle,
            HostAddress _local_addr,
            std::string _name, std::uint8_t _family, util_socket_t _fd,
            HostAddress _peer_addr);

        void SetState(SessionState _state);
        auto Event() -> Ptr<io::Event>&;
        auto Socket() -> net::Socket&;
        void SetDestroy(SessionDestroy _destroy);

        void HandleCallback(const Ptr<io::Event>& _event, std::uint8_t _events, const Timestamp& _receive_time);

        void HandleRead(const Timestamp&);
        void HandleWrite();
        void HandleClose();
        void HandleError();

    private:
        void ConnectEstablished();

        friend class TcpClient;
        friend class TcpServe;
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_TCP_SESSION_H_