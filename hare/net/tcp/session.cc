#include "hare/base/fwd-inl.h"
#include "hare/base/io/reactor.h"
#include <hare/base/io/socket_op.h>
#include <hare/net/tcp/session.h>

#include <cassert>

#define DEFAULT_HIGH_WATER (64UL * 1024 * 1024)

namespace hare {
namespace net {

    namespace detail {

        static auto StateToString(SessionState state) -> const char*
        {
            switch (state) {
            case STATE_CONNECTING:
                return "STATE_CONNECTING";
            case STATE_CONNECTED:
                return "STATE_CONNECTED";
            case STATE_DISCONNECTING:
                return "STATE_DISCONNECTING";
            case STATE_DISCONNECTED:
                return "STATE_DISCONNECTED";
            default:
                return "UNKNOWN STATE";
            }
        }
    } // namespace detail

    HARE_IMPL_DEFAULT(
        TcpSession,
        std::string name {};
        io::Cycle * cycle { nullptr };
        Ptr<io::Event> event { nullptr };
        Socket socket;

        const HostAddress local_addr {};
        const HostAddress peer_addr {};

        bool reading { false };
        SessionState state { STATE_CONNECTING };

        Buffer out_buffer {};
        Buffer in_buffer {};

        std::size_t high_water_mark { DEFAULT_HIGH_WATER };
        TcpSession::WriteCallback write {};
        TcpSession::HighWaterCallback high_water {};
        TcpSession::ReadRallback read {};
        TcpSession::ConnectCallback connect {};
        TcpSession::SessionDestroy destroy {};

        util::Any any_ctx {};

        TcpSessionImpl(HostAddress _local_addr,
            std::uint8_t _family, util_socket_t _fd,
            HostAddress _peer_addr)
            : socket(_family, TYPE_TCP, _fd)
            , local_addr(std::move(_local_addr))
            , peer_addr(std::move(_peer_addr)) 
            {}
    )

    TcpSession::~TcpSession()
    {
        assert(d_ptr(impl_)->state == STATE_DISCONNECTED);
        HARE_INTERNAL_TRACE("session[{}] at {} fd={} free.", d_ptr(impl_)->name, (void*)this, Fd());
        delete impl_;
    }

    auto TcpSession::Name() const -> std::string
    {
        return d_ptr(impl_)->name;
    }
    auto TcpSession::OwnerCycle() const -> io::Cycle*
    {
        return d_ptr(impl_)->cycle;
    }
    auto TcpSession::LocalAddress() const -> const HostAddress&
    {
        return d_ptr(impl_)->local_addr;
    }
    auto TcpSession::PeerAddress() const -> const HostAddress&
    {
        return d_ptr(impl_)->peer_addr;
    }
    auto TcpSession::State() const -> SessionState
    {
        return d_ptr(impl_)->state;
    }
    auto TcpSession::Fd() const -> util_socket_t
    {
        return d_ptr(impl_)->socket.fd();
    }

    void TcpSession::SetHighWaterMark(std::size_t _hwm)
    {
        d_ptr(impl_)->high_water_mark = _hwm;
    }
    void TcpSession::SetReadCallback(ReadRallback _read)
    {
        d_ptr(impl_)->read = std::move(_read);
    }
    void TcpSession::SetWriteCallback(WriteCallback _write)
    {
        d_ptr(impl_)->write = std::move(_write);
    }
    void TcpSession::SetHighWaterCallback(HighWaterCallback _high_water)
    {
        d_ptr(impl_)->high_water = std::move(_high_water);
    }
    void TcpSession::SetConnectCallback(ConnectCallback _connect)
    {
        d_ptr(impl_)->connect = std::move(_connect);
    }

    void TcpSession::SetContext(const util::Any& context)
    {
        d_ptr(impl_)->any_ctx = context;
    }
    auto TcpSession::GetContext() const -> const util::Any&
    {
        return d_ptr(impl_)->any_ctx;
    }

    auto TcpSession::Shutdown() -> Error
    {
        if (d_ptr(impl_)->state == STATE_CONNECTED) {
            SetState(STATE_DISCONNECTING);
            if (!d_ptr(impl_)->event->Writing()) {
                return d_ptr(impl_)->socket.ShutdownWrite();
            }
            return Error(ERROR_SOCKET_WRITING);
        }
        return Error(ERROR_SESSION_ALREADY_DISCONNECT);
    }

    auto TcpSession::ForceClose() -> Error
    {
        if (d_ptr(impl_)->state == STATE_CONNECTED || d_ptr(impl_)->state == STATE_DISCONNECTING) {
            HandleClose();
            return Error(ERROR_SUCCESS);
        }
        return Error(ERROR_SESSION_ALREADY_DISCONNECT);
    }

    void TcpSession::StartRead()
    {
        if (!d_ptr(impl_)->reading || !d_ptr(impl_)->event->Reading()) {
            d_ptr(impl_)->event->EnableRead();
            d_ptr(impl_)->reading = true;
        }
    }

    void TcpSession::StopRead()
    {
        if (d_ptr(impl_)->reading || d_ptr(impl_)->event->Reading()) {
            d_ptr(impl_)->event->DisableRead();
            d_ptr(impl_)->reading = false;
        }
    }

    auto TcpSession::Append(Buffer& _buffer) -> bool
    {
        if (State() == STATE_CONNECTED) {
            auto tmp = std::make_shared<Buffer>();
            tmp->Append(_buffer);
            OwnerCycle()->QueueInCycle(std::bind([](const WPtr<TcpSession>& session, hare::Ptr<Buffer>& buffer) {
                auto tcp = session.lock();
                if (tcp) {
                    auto out_buffer_size = d_ptr(tcp->impl_)->out_buffer.Size();
                    d_ptr(tcp->impl_)->out_buffer.Append(*buffer);
                    if (out_buffer_size == 0) {
                        tcp->Event()->EnableWrite();
                        tcp->HandleWrite();
                    } else if (out_buffer_size > d_ptr(tcp->impl_)->high_water_mark && d_ptr(tcp->impl_)->high_water_mark) {
                        d_ptr(tcp->impl_)->high_water(tcp);
                    } else if (out_buffer_size > d_ptr(tcp->impl_)->high_water_mark) {
                        HARE_INTERNAL_ERROR("high_water_mark_callback has not been set for tcp-session[{}].", tcp->Name());
                    }
                }
            },
                shared_from_this(), std::move(tmp)));
            return true;
        }
        return false;
    }

    auto TcpSession::Send(const void* _bytes, std::size_t _length) -> bool
    {
        if (State() == STATE_CONNECTED) {
            auto tmp = std::make_shared<Buffer>();
            tmp->Add(_bytes, _length);
            OwnerCycle()->QueueInCycle(std::bind([](const WPtr<TcpSession>& session, hare::Ptr<Buffer>& buffer) {
                auto tcp = session.lock();
                if (tcp) {
                    auto out_buffer_size = d_ptr(tcp->impl_)->out_buffer.Size();
                    d_ptr(tcp->impl_)->out_buffer.Append(*buffer);
                    if (out_buffer_size == 0) {
                        tcp->Event()->EnableWrite();
                        tcp->HandleWrite();
                    } else if (out_buffer_size > d_ptr(tcp->impl_)->high_water_mark && d_ptr(tcp->impl_)->high_water_mark) {
                        d_ptr(tcp->impl_)->high_water(tcp);
                    } else if (out_buffer_size > d_ptr(tcp->impl_)->high_water_mark) {
                        HARE_INTERNAL_ERROR("high_water_mark_callback has not been set for tcp-session[{}].", tcp->Name());
                    }
                }
            },
                shared_from_this(), std::move(tmp)));
            return true;
        }
        return false;
    }

    auto TcpSession::SetTcpNoDelay(bool _on) -> Error
    {
        return Socket().SetTcpNoDelay(_on);
    }

    TcpSession::TcpSession(io::Cycle* _cycle,
        HostAddress _local_addr,
        std::string _name, std::uint8_t _family, util_socket_t _fd,
        HostAddress _peer_addr)
        : impl_(new TcpSessionImpl(std::move(_local_addr), _family, _fd, std::move(_peer_addr)))
    {
        d_ptr(impl_)->cycle = CHECK_NULL(_cycle);
        d_ptr(impl_)->event.reset(new io::Event(_fd,
            std::bind(&TcpSession::HandleCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            SESSION_READ | SESSION_WRITE | io::EVENT_PERSIST,
            0));
        d_ptr(impl_)->name = std::move(_name);
    }

    void TcpSession::SetState(SessionState _state)
    {
        d_ptr(impl_)->state = _state;
    }
    auto TcpSession::Event() -> Ptr<io::Event>&
    {
        return d_ptr(impl_)->event;
    }
    auto TcpSession::Socket() -> net::Socket&
    {
        return d_ptr(impl_)->socket;
    }
    void TcpSession::SetDestroy(SessionDestroy _destroy)
    {
        d_ptr(impl_)->destroy = std::move(_destroy);
    }

    void TcpSession::HandleCallback(const Ptr<io::Event>& _event, std::uint8_t _events, const Timestamp& _receive_time)
    {
        OwnerCycle()->AssertInCycleThread();
        assert(_event == d_ptr(impl_)->event);
        HARE_INTERNAL_TRACE("session[{}] revents: {}.", d_ptr(impl_)->event->fd(), _events);
        if (CHECK_EVENT(_events, SESSION_READ)) {
            HandleRead(_receive_time);
        }
        if (CHECK_EVENT(_events, SESSION_WRITE)) {
            HandleWrite();
        }
        if (CHECK_EVENT(_events, SESSION_CLOSED)) {
            HandleClose();
        }
    }

    void TcpSession::HandleRead(const Timestamp& _time)
    {
        auto read_n = d_ptr(impl_)->in_buffer.Read(Fd(), -1);
        if (read_n == 0) {
            HandleClose();
        } else if (read_n > 0 && d_ptr(impl_)->read) {
            d_ptr(impl_)->read(shared_from_this(), d_ptr(impl_)->in_buffer, _time);
        } else {
            if (read_n > 0) {
                HARE_INTERNAL_ERROR("read_callback has not been set for tcp-session[{}].", Name());
            }
            HandleError();
        }
    }

    void TcpSession::HandleWrite()
    {
        if (Event()->Writing()) {
            auto write_n = d_ptr(impl_)->out_buffer.Write(Fd(), -1);
            if (write_n >= 0) {
                if (d_ptr(impl_)->out_buffer.Size() == 0) {
                    Event()->DisableWrite();
                    OwnerCycle()->QueueInCycle(std::bind([=](const WPtr<TcpSession>& session) {
                        auto tcp = session.lock();
                        if (tcp) {
                            if (d_ptr(impl_)->write) {
                                d_ptr(impl_)->write(tcp);
                            } else {
                                HARE_INTERNAL_ERROR("write_callback has not been set for tcp-session[{}].", Name());
                            }
                        }
                    },
                        shared_from_this()));
                }
                if (State() == STATE_DISCONNECTING) {
                    HandleClose();
                }
            } else {
                HARE_INTERNAL_ERROR("an error occurred while writing the socket, detail: {}.", socket_op::SocketErrorInfo(Fd()));
            }
        } else {
            HARE_INTERNAL_TRACE("tcp-session[fd={}, name={}] is down, no more writing.", Fd(), Name());
        }
    }

    void TcpSession::HandleClose()
    {
        HARE_INTERNAL_TRACE("fd={} state={}.", Fd(), detail::StateToString(d_ptr(impl_)->state));
        assert(d_ptr(impl_)->state == STATE_CONNECTED || d_ptr(impl_)->state == STATE_DISCONNECTING);
        // we don't close fd, leave it to dtor, so we can find leaks easily.
        SetState(STATE_DISCONNECTED);
        d_ptr(impl_)->event->DisableRead();
        d_ptr(impl_)->event->DisableWrite();
        if (d_ptr(impl_)->connect) {
            d_ptr(impl_)->connect(shared_from_this(), SESSION_CLOSED);
        } else {
            HARE_INTERNAL_ERROR("connect_callback has not been set for session[{}], session is closed.", d_ptr(impl_)->name);
        }
        d_ptr(impl_)->event->Deactivate();
        d_ptr(impl_)->destroy();
    }

    void TcpSession::HandleError()
    {
        if (d_ptr(impl_)->connect) {
            d_ptr(impl_)->connect(shared_from_this(), SESSION_ERROR);
        } else {
            HARE_INTERNAL_ERROR("occurred error to the session[{}], detail: {}.",
                d_ptr(impl_)->name, socket_op::SocketErrorInfo(Fd()));
        }
    }

    void TcpSession::ConnectEstablished()
    {
        assert(d_ptr(impl_)->state == STATE_CONNECTING);
        SetState(STATE_CONNECTED);
        if (d_ptr(impl_)->connect) {
            d_ptr(impl_)->connect(shared_from_this(), SESSION_CONNECTED);
        } else {
            HARE_INTERNAL_ERROR("connect_callback has not been set for session[{}], session connected.", d_ptr(impl_)->name);
        }
        d_ptr(impl_)->event->Tie(shared_from_this());
        d_ptr(impl_)->cycle->EventUpdate(d_ptr(impl_)->event);
        d_ptr(impl_)->event->EnableRead();
    }

} // namespace net
} // namespace hare