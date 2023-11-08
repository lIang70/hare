#include <hare/base/io/timer.h>

#include "base/fwd_inl.h"
#include "base/io/reactor.h"

namespace hare {

// clang-format off
HARE_IMPL_DEFAULT(Timer, 
  Task task{};
  void TimerCallback(const Event* _event, std::uint8_t _events,
                                     const Timestamp& _ts) const;
)
// clang-format on

void TimerImpl::TimerCallback(const Event* _event, std::uint8_t _events,
                              const Timestamp& _ts) const {
  IgnoreUnused(_event);
  IgnoreUnused(_ts);
  if (CHECK_EVENT(_events, EVENT_TIMEOUT) == 0) {
    HARE_INTERNAL_ERROR("cannot check event[{}] with \'EVENT_TIMEOUT\'.",
                        _event->id());
  }
  task();
}

Timer::Timer(Task _task, std::int64_t _timeval, bool is_persist)
    : Event(-1,
            std::bind(
                [this](const Event* event, std::uint8_t events,
                       const Timestamp& timestamp) {
                  IMPL->TimerCallback(event, events, timestamp);
                },
                std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3),
            is_persist ? (EVENT_TIMEOUT | EVENT_PERSIST) : EVENT_TIMEOUT,
            _timeval),
      impl_(new TimerImpl) {
  IMPL->task = std::move(_task);
}

Timer::~Timer() {
  Cancel();
  delete impl_;
}

void Timer::Cancel() {
  if (cycle() != nullptr) {
    cycle()->Cancel(Event::id());
  }
}

}  // namespace hare