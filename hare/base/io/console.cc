#include <hare/base/io/console.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/hare-config.h>

#include <map>

#include "base/fwd-inl.h"
#include "base/io/reactor.h"

#if HARE__HAVE_UNISTD_H
#include <unistd.h>
#endif  // HARE__HAVE_UNISTD_H

#if HARE__HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif  // HARE__HAVE_SYS_SOCKET_H

#if defined(H_OS_WIN32)
#define STDIN_FILENO 0
#define read _read
#endif  // H_OS_WIN32

namespace hare {

namespace io_inner {
static void GlobalConsoleHandle(const std::string& _command_line) {
  HARE_INTERNAL_ERROR(
      "unregistered command[{}], you can register \"default handle\" to "
      "console for handling all command.",
      _command_line);
}
}  // namespace io_inner

// clang-format off
HARE_IMPL_DEFAULT(Console,
  Ptr<Event> console_event{nullptr};
  std::map<std::string, Task> handlers{};
  AtomicHook<Console::DefaultHandle> default_handle{
    io_inner::GlobalConsoleHandle};
  bool attached{false};
)
// clang-format on

auto Console::Instance() -> Console& {
  static Console static_console{};
  return static_console;
}

Console::~Console() {
  IMPL->console_event->Tie(nullptr);
  delete impl_;
}

void Console::RegisterDefaultHandle(DefaultHandle _default_handle) {
  IMPL->default_handle.Store(_default_handle);
}

void Console::RegisterHandle(std::string _handle_mask, Task _handle) {
  HARE_ASSERT(!IMPL->attached);
  IMPL->handlers.emplace(_handle_mask, _handle);
}

auto Console::Attach(Cycle* _cycle) -> bool {
  if (!_cycle) {
    return false;
  }
  if (_cycle->is_running()) {
    auto in_cycle_thread = _cycle->InCycleThread();
    auto cdl = std::make_shared<CountDownLatch>(1);
    _cycle->RunInCycle([=] {
      _cycle->EventUpdate(IMPL->console_event);
      cdl->CountDown();
    });

    if (!in_cycle_thread) {
      cdl->Await();
    }

  } else {
    _cycle->EventUpdate(IMPL->console_event);
  }

  IMPL->console_event->Tie(IMPL->console_event);
  IMPL->attached = true;
  return true;
}

Console::Console() : impl_(new ConsoleImpl) {
  IMPL->console_event.reset(
      new Event(STDIN_FILENO,
                std::bind(&Console::Process, this, std::placeholders::_1,
                          std::placeholders::_2, std::placeholders::_3),
                EVENT_READ | EVENT_PERSIST | EVENT_ET, 0));
  IMPL->default_handle.Store(io_inner::GlobalConsoleHandle);
}

void Console::Process(const Ptr<Event>& _event, std::uint8_t _events,
                      const Timestamp& _receive_time) {
  HARE_ASSERT(IMPL->console_event == _event);
  if (CHECK_EVENT(_events, EVENT_READ) == 0) {
    HARE_INTERNAL_ERROR("cannot check EVENT_READ.");
    return;
  }

  std::array<char, HARE_SMALL_BUFFER> console_line{};

  auto len = ::read(_event->fd(), console_line.data(), HARE_SMALL_BUFFER);

  if (len < 0) {
    HARE_INTERNAL_ERROR("cannot read from STDIN.");
    return;
  }
  std::string line(console_line.data(), len);
  while (line.back() == '\n') {
    line.pop_back();
  }

  HARE_INTERNAL_TRACE("recv console input[{}] in {}.", line,
                      _receive_time.ToFmt(true));

  auto iter = IMPL->handlers.find(line);
  if (iter != IMPL->handlers.end()) {
    iter->second();
  } else {
    IMPL->default_handle(line);
  }
}

}  // namespace hare
