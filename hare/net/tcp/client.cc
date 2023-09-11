#include <hare/base/io/cycle.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/net/tcp/client.h>

#include "base/fwd-inl.h"
#include "socket_op.h"

namespace hare {
namespace net {

HARE_IMPL_DEFAULT(TcpClient, std::string name{};

                  io::Cycle * cycle{}; bool started{false};

                  TcpSession::ConnectCallback connect_cb{};

                  Ptr<TcpSession> session{}; io::Event::Id retry_id{-1};)

TcpClient::TcpClient(io::Cycle* _cycle, std::string _name)
    : impl_(new TcpClientImpl) {
  IMPL->name = std::move(_name);
  IMPL->cycle = CHECK_NULL(_cycle);
}

TcpClient::~TcpClient() {
  Close();
  delete impl_;
}

auto TcpClient::MainCycle() const -> io::Cycle* { return IMPL->cycle; }

auto TcpClient::IsRunning() const -> bool { return IMPL->started; }

void TcpClient::SetConnectCallback(TcpSession::ConnectCallback _connect) {
  IMPL->connect_cb = std::move(_connect);
}

auto TcpClient::Connected() -> bool {
  auto ret{false};
  if (!MainCycle()->is_running()) {
    return ret;
  }

  util::CountDownLatch cdl{1};
  auto in_thread = MainCycle()->InCycleThread();

  MainCycle()->RunInCycle([&] {
    if (IMPL->session) {
      ret = IMPL->session->Connected();
    }
    cdl.CountDown();
  });

  if (!in_thread) {
    cdl.Await();
  }

  return ret;
}

void TcpClient::ConnectTo(const HostAddress& _address, bool _retry,
                          std::int32_t _retry_times, std::int64_t _delay) {
  if (Connected()) {
    HARE_INTERNAL_ERROR(
        "the client is connected, please reconnect after disconnecting.");
    return;
  }
  auto peer_addr = _address.Clone();
  MainCycle()->QueueInCycle([=] {
    auto family = peer_addr->Family();
    auto fd = socket_op::CreateNonblockingOrDie(family);
    auto ret = socket_op::Connect(fd, peer_addr->get_sockaddr(),
                                  socket_op::AddrLen(family));
    auto saved_errno = !ret ? errno : 0;
    switch (saved_errno) {
      case 0:
      case EINPROGRESS:
      case EINTR:
      case EISCONN:
        IMPL->session.reset(new TcpSession(
            IMPL->cycle, HostAddress::LocalAddress(fd),
            fmt::format("{}-{}#tcp", IMPL->name, peer_addr->ToIpPort()),
            peer_addr->Family(), fd, std::move(*peer_addr)));
        IMPL->session->SetDestroy(std::bind(&TcpClient::DestroySession, this));
        IMPL->connect_cb(IMPL->session, SESSION_CONNECTED);
        IMPL->session->ConnectEstablished();
        IMPL->retry_id = -1;
        break;

      case EAGAIN:
      case EADDRINUSE:
      case EADDRNOTAVAIL:
      case ECONNREFUSED:
      case ENETUNREACH:
        socket_op::Close(fd);
        if (_retry && _retry_times > 0) {
          IMPL->retry_id = IMPL->cycle->RunAfter(
              [=]() mutable {
                this->ConnectTo(*peer_addr, _retry, --_retry_times, _delay);
              },
              _delay);
        } else if (_retry) {
          HARE_INTERNAL_ERROR("cannot connect to {}", peer_addr->ToIpPort());
          IMPL->retry_id = -1;
        }
        break;

      case EACCES:
      case EPERM:
      case EAFNOSUPPORT:
      case EALREADY:
      case EBADF:
      case EFAULT:
      case ENOTSOCK:
        socket_op::Close(fd);
        HARE_INTERNAL_ERROR(
            "a connection error {} occurred while connecting to {}",
            saved_errno, peer_addr->ToIpPort());
        IMPL->retry_id = -1;
        break;

      default:
        socket_op::Close(fd);
        HARE_INTERNAL_ERROR(
            "a unexpected error {} occurred while connecting to {}",
            saved_errno, peer_addr->ToIpPort());
        IMPL->retry_id = -1;
        break;
    }
  });
}

void TcpClient::Close() {
  util::CountDownLatch cdl{1};
  MainCycle()->RunInCycle([&] {
    if (IMPL->session) {
      auto ret = IMPL->session->ForceClose();
      if (!ret) {
        HARE_INTERNAL_ERROR("{}", ret.Description());
      }
    }
    cdl.CountDown();
  });

  if (MainCycle()->InCycleThread()) {
    cdl.Await();
  }
}

auto TcpClient::Send(const void* _buffer, std::size_t _len) -> bool {
  if (!Connected()) {
    return false;
  }
  Buffer buffer{};
  auto ret = buffer.Add(_buffer, _len);
  if (!ret) {
    return ret;
  }

  util::CountDownLatch cdl{1};

  MainCycle()->RunInCycle([&] {
    if (IMPL->session) {
      ret = IMPL->session->Append(buffer);
    } else {
      ret = false;
    }
    cdl.CountDown();
  });

  if (!MainCycle()->InCycleThread()) {
    cdl.Await();
  }

  return ret;
}

auto TcpClient::Append(Buffer& _buffer) -> bool {
  if (!Connected()) {
    return false;
  }

  auto ret{false};
  util::CountDownLatch cdl{1};

  MainCycle()->RunInCycle([&] {
    if (IMPL->session) {
      ret = IMPL->session->Append(_buffer);
    } else {
      ret = false;
    }
    cdl.CountDown();
  });

  if (!MainCycle()->InCycleThread()) {
    cdl.Await();
  }

  return ret;
}

void TcpClient::DestroySession() {
  IMPL->cycle->AssertInCycleThread();
  if (IMPL->session) {
    IMPL->session.reset();
  }
}

}  // namespace net
}  // namespace hare