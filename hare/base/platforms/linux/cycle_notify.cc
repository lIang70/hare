#include "base/io/cycle_notify.h"

#include "base/fwd_inl.h"
#include "base/io/operation_inl.h"
#include "base/io/reactor.h"

#ifdef H_OS_LINUX

#include <sys/socket.h>

#if HARE__HAVE_EVENTFD
#include <sys/eventfd.h>
#endif

namespace hare {

#if HARE__HAVE_EVENTFD
static auto CreateNotifier() -> util_socket_t {
  auto evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    HARE_INTERNAL_FATAL("failed to get event fd in ::eventfd.");
  }
  return evtfd;
}

CycleNotify::CycleNotify()
    : Event(CreateNotifier(),
            std::bind(&CycleNotify::EventCallback, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3),
            EVENT_READ | EVENT_PERSIST, 0) {}

CycleNotify::~CycleNotify() { ::hare::io::Close(fd()); }

#else

static auto CreateNotifier(util_socket_t& _write_fd) -> util_socket_t {
  util_socket_t fd[2];
  auto ret = io::Socketpair(AF_INET, SOCK_STREAM, 0, fd);
  if (ret < 0) {
    HARE_INTERNAL_FATAL("fail to create socketpair for notify_fd.");
  }
  _write_fd = fd[0];
  return fd[1];
}

CycleNotify::CycleNotify()
    : Event(CreateNotifier(write_fd_),
            std::bind(&CycleNotify::EventCallback, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3),
            EVENT_READ | EVENT_PERSIST, 0) {}

CycleNotify::~CycleNotify() {
  ::hare::io::Close(write_fd_);
  ::hare::io::Close(fd());
}

#endif

void CycleNotify::SendNotify() const {
  std::uint64_t one = 1;
#if HARE__HAVE_EVENTFD
  auto write_n = ::hare::io::Write(fd(), &one, sizeof(one));
#else
  auto write_n = ::hare::io::Write(write_fd_, &one, sizeof(one));
#endif
  if (write_n != sizeof(one)) {
    HARE_INTERNAL_ERROR("write[{} B] instead of {} B", write_n, sizeof(one));
  }
}

void CycleNotify::EventCallback(const Event* _event, std::uint8_t _events,
                                const Timestamp& _receive_time) {
  IgnoreUnused(_event, _receive_time);
  HARE_ASSERT(_event == this);

  if (CHECK_EVENT(_events, EVENT_READ) != 0) {
    std::uint64_t one = 0;
    auto read_n = ::hare::io::Read((int)fd(), &one, sizeof(one));
    if (read_n != sizeof(one) && one != static_cast<std::uint64_t>(1)) {
      HARE_INTERNAL_ERROR("read notify[{} B] instead of {} B", read_n,
                          sizeof(one));
    }
  } else {
    HARE_INTERNAL_FATAL("an error occurred while accepting notify in fd[{}]",
                        fd());
  }
}

}  // namespace hare

#endif  // H_OS_LINUX