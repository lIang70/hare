#include <hare/base/io/operation.h>
#include <hare/net/tcp/session.h>

#include "base/fwd-inl.h"
#include "base/io/reactor.h"

#define DEFAULT_HIGH_WATER (64UL * 1024 * 1024)

namespace hare {
namespace net {

namespace detail {

static auto StateToString(SessionState state) -> const char* {
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
}  // namespace detail

HARE_IMPL_DEFAULT(TcpSession, std::string name{}; io::Cycle * cycle{nullptr};
                  Ptr<io::Event> event{nullptr}; Socket socket;

                  const HostAddress local_addr{}; const HostAddress peer_addr{};

                  bool reading{false}; SessionState state{STATE_CONNECTING};

                  Buffer out_buffer{}; Buffer in_buffer{};

                  std::size_t high_water_mark{DEFAULT_HIGH_WATER};
                  TcpSession::WriteCallback write{};
                  TcpSession::HighWaterCallback high_water{};
                  TcpSession::ReadRallback read{};
                  TcpSession::ConnectCallback connect{};
                  TcpSession::SessionDestroy destroy{};

                  util::Any any_ctx{};

                  TcpSessionImpl(HostAddress _local_addr, std::uint8_t _family,
                                 util_socket_t _fd, HostAddress _peer_addr)
                  : socket(_family, TYPE_TCP, _fd),
                    local_addr(std::move(_local_addr)),
                    peer_addr(std::move(_peer_addr)){})

TcpSession::~TcpSession() {
  HARE_ASSERT(IMPL->state == STATE_DISCONNECTED);
  HARE_INTERNAL_TRACE("session[{}] at {} fd={} free.", IMPL->name, (void*)this,
                      Fd());
  delete impl_;
}

auto TcpSession::Name() const -> std::string { return IMPL->name; }
auto TcpSession::OwnerCycle() const -> io::Cycle* { return IMPL->cycle; }
auto TcpSession::LocalAddress() const -> const HostAddress& {
  return IMPL->local_addr;
}
auto TcpSession::PeerAddress() const -> const HostAddress& {
  return IMPL->peer_addr;
}
auto TcpSession::State() const -> SessionState { return IMPL->state; }
auto TcpSession::Fd() const -> util_socket_t { return IMPL->socket.fd(); }

void TcpSession::SetHighWaterMark(std::size_t _hwm) {
  IMPL->high_water_mark = _hwm;
}
void TcpSession::SetReadCallback(ReadRallback _read) {
  IMPL->read = std::move(_read);
}
void TcpSession::SetWriteCallback(WriteCallback _write) {
  IMPL->write = std::move(_write);
}
void TcpSession::SetHighWaterCallback(HighWaterCallback _high_water) {
  IMPL->high_water = std::move(_high_water);
}
void TcpSession::SetConnectCallback(ConnectCallback _connect) {
  IMPL->connect = std::move(_connect);
}

void TcpSession::SetContext(const util::Any& context) {
  IMPL->any_ctx = context;
}
auto TcpSession::GetContext() const -> const util::Any& {
  return IMPL->any_ctx;
}

auto TcpSession::Shutdown() -> Error {
  if (IMPL->state == STATE_CONNECTED) {
    SetState(STATE_DISCONNECTING);
    if (!IMPL->event->Writing()) {
      return IMPL->socket.ShutdownWrite();
    }
    return Error(ERROR_SOCKET_WRITING);
  }
  return Error(ERROR_SESSION_ALREADY_DISCONNECT);
}

auto TcpSession::ForceClose() -> Error {
  if (IMPL->state == STATE_CONNECTED || IMPL->state == STATE_DISCONNECTING) {
    HandleClose();
    return Error(ERROR_SUCCESS);
  }
  return Error(ERROR_SESSION_ALREADY_DISCONNECT);
}

void TcpSession::StartRead() {
  if (!IMPL->reading || !IMPL->event->Reading()) {
    IMPL->event->EnableRead();
    IMPL->reading = true;
  }
}

void TcpSession::StopRead() {
  if (IMPL->reading || IMPL->event->Reading()) {
    IMPL->event->DisableRead();
    IMPL->reading = false;
  }
}

auto TcpSession::Append(Buffer& _buffer) -> bool {
  if (State() == STATE_CONNECTED) {
    auto tmp = std::make_shared<Buffer>();
    tmp->Append(_buffer);
    OwnerCycle()->QueueInCycle(std::bind(
        [](const WPtr<TcpSession>& session, hare::Ptr<Buffer>& buffer) {
          auto tcp = session.lock();
          if (tcp) {
            auto out_buffer_size = d_ptr(tcp->impl_)->out_buffer.Size();
            d_ptr(tcp->impl_)->out_buffer.Append(*buffer);
            if (out_buffer_size == 0) {
              tcp->Event()->EnableWrite();
              tcp->HandleWrite();
            } else if (out_buffer_size > d_ptr(tcp->impl_)->high_water_mark &&
                       d_ptr(tcp->impl_)->high_water_mark) {
              d_ptr(tcp->impl_)->high_water(tcp);
            } else if (out_buffer_size > d_ptr(tcp->impl_)->high_water_mark) {
              HARE_INTERNAL_ERROR(
                  "high_water_mark_callback has not been set for "
                  "tcp-session[{}].",
                  tcp->Name());
            }
          }
        },
        shared_from_this(), std::move(tmp)));
    return true;
  }
  return false;
}

auto TcpSession::Send(const void* _bytes, std::size_t _length) -> bool {
  if (State() == STATE_CONNECTED) {
    auto tmp = std::make_shared<Buffer>();
    tmp->Add(_bytes, _length);
    OwnerCycle()->QueueInCycle(std::bind(
        [](const WPtr<TcpSession>& session, hare::Ptr<Buffer>& buffer) {
          auto tcp = session.lock();
          if (tcp) {
            auto out_buffer_size = d_ptr(tcp->impl_)->out_buffer.Size();
            d_ptr(tcp->impl_)->out_buffer.Append(*buffer);
            if (out_buffer_size == 0) {
              tcp->Event()->EnableWrite();
              tcp->HandleWrite();
            } else if (out_buffer_size > d_ptr(tcp->impl_)->high_water_mark &&
                       d_ptr(tcp->impl_)->high_water_mark) {
              d_ptr(tcp->impl_)->high_water(tcp);
            } else if (out_buffer_size > d_ptr(tcp->impl_)->high_water_mark) {
              HARE_INTERNAL_ERROR(
                  "high_water_mark_callback has not been set for "
                  "tcp-session[{}].",
                  tcp->Name());
            }
          }
        },
        shared_from_this(), std::move(tmp)));
    return true;
  }
  return false;
}

TcpSession::TcpSession(io::Cycle* _cycle, HostAddress _local_addr,
                       std::string _name, std::uint8_t _family,
                       util_socket_t _fd, HostAddress _peer_addr)
    : impl_(new TcpSessionImpl(std::move(_local_addr), _family, _fd,
                               std::move(_peer_addr))) {
  IMPL->cycle = CHECK_NULL(_cycle);
  IMPL->event.reset(new io::Event(
      _fd,
      std::bind(&TcpSession::HandleCallback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3),
      SESSION_READ | SESSION_WRITE | io::EVENT_PERSIST, 0));
  IMPL->name = std::move(_name);
}

void TcpSession::SetState(SessionState _state) { IMPL->state = _state; }
auto TcpSession::Event() -> Ptr<io::Event>& { return IMPL->event; }
auto TcpSession::Socket() -> net::Socket& { return IMPL->socket; }
void TcpSession::SetDestroy(SessionDestroy _destroy) {
  IMPL->destroy = std::move(_destroy);
}

void TcpSession::HandleCallback(const Ptr<io::Event>& _event,
                                std::uint8_t _events,
                                const Timestamp& _receive_time) {
  OwnerCycle()->AssertInCycleThread();
  HARE_ASSERT(_event == IMPL->event);
  HARE_INTERNAL_TRACE("session[{}] revents: {}.", IMPL->event->fd(), _events);
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

void TcpSession::HandleRead(const Timestamp& _time) {
  auto read_n = IMPL->in_buffer.Read(Fd(), -1);
  if (read_n == 0) {
    HandleClose();
  } else if (read_n > 0 && IMPL->read) {
    IMPL->read(shared_from_this(), IMPL->in_buffer, _time);
  } else {
    if (read_n > 0) {
      HARE_INTERNAL_ERROR("read_callback has not been set for tcp-session[{}].",
                          Name());
    }
    HandleError();
  }
}

void TcpSession::HandleWrite() {
  if (Event()->Writing()) {
    auto write_n = IMPL->out_buffer.Write(Fd(), -1);
    if (write_n >= 0) {
      if (IMPL->out_buffer.Size() == 0) {
        Event()->DisableWrite();
        OwnerCycle()->QueueInCycle(std::bind(
            [=](const WPtr<TcpSession>& session) {
              auto tcp = session.lock();
              if (tcp) {
                if (IMPL->write) {
                  IMPL->write(tcp);
                } else {
                  HARE_INTERNAL_ERROR(
                      "write_callback has not been set for tcp-session[{}].",
                      Name());
                }
              }
            },
            shared_from_this()));
      }
      if (State() == STATE_DISCONNECTING) {
        HandleClose();
      }
    } else {
      HARE_INTERNAL_ERROR(
          "an error occurred while writing the socket, detail: {}.",
          io::SocketErrorInfo(Fd()));
    }
  } else {
    HARE_INTERNAL_TRACE("tcp-session[fd={}, name={}] is down, no more writing.",
                        Fd(), Name());
  }
}

void TcpSession::HandleClose() {
  HARE_INTERNAL_TRACE("fd={} state={}.", Fd(),
                      detail::StateToString(IMPL->state));
  HARE_ASSERT(IMPL->state == STATE_CONNECTED ||
              IMPL->state == STATE_DISCONNECTING);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  SetState(STATE_DISCONNECTED);
  IMPL->event->DisableRead();
  IMPL->event->DisableWrite();
  if (IMPL->connect) {
    IMPL->connect(shared_from_this(), SESSION_CLOSED);
  } else {
    HARE_INTERNAL_ERROR(
        "connect_callback has not been set for session[{}], session is closed.",
        IMPL->name);
  }
  IMPL->event->Deactivate();
  HARE_ASSERT(IMPL->destroy);
  IMPL->destroy();
}

void TcpSession::HandleError() {
  if (IMPL->connect) {
    IMPL->connect(shared_from_this(), SESSION_ERROR);
  } else {
    HARE_INTERNAL_ERROR("occurred error to the session[{}], detail: {}.",
                        IMPL->name, io::SocketErrorInfo(Fd()));
  }
}

void TcpSession::ConnectEstablished() {
  HARE_ASSERT(IMPL->state == STATE_CONNECTING);
  SetState(STATE_CONNECTED);
  if (IMPL->connect) {
    IMPL->connect(shared_from_this(), SESSION_CONNECTED);
  } else {
    HARE_INTERNAL_ERROR(
        "connect_callback has not been set for session[{}], session connected.",
        IMPL->name);
  }
  IMPL->event->Tie(shared_from_this());
  IMPL->cycle->EventUpdate(IMPL->event);
  IMPL->event->EnableRead();
}

}  // namespace net
}  // namespace hare