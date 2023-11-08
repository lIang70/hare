#include <hare/base/exception.h>
#include <hare/base/io/timer.h>
#include <hare/base/util/count_down_latch.h>

#include <mutex>

#include "base/fwd_inl.h"
#include "base/io/cycle_notify.h"
#include "base/io/reactor.h"
#include "base/thread/store.h"

namespace hare {

namespace cycle_inner {

static auto GetWaitTime(const PriorityTimer& _ptimer) -> std::int32_t {
  if (_ptimer.empty()) {
    return POLL_TIME_MICROSECONDS;
  }
  auto time =
      static_cast<std::int32_t>(_ptimer.top().stamp.microseconds_since_epoch() -
                                Timestamp::Now().microseconds_since_epoch());

  return time <= 0 ? 1 : Min(time, POLL_TIME_MICROSECONDS);
}

static void PrintActiveEvents(const Event* _active_event) {
  HARE_INTERNAL_TRACE("event[{}] debug info: {}.", _active_event->fd(),
                      _active_event->EventToString());
  IgnoreUnused(_active_event);
}

}  // namespace cycle_inner

// clang-format off
HARE_IMPL_DEFAULT(Cycle, 
  Timestamp reactor_time{}; 
  std::uint64_t tid{0};
  bool is_running{false};
  bool quit{false};
  bool event_handling{false};
  bool calling_pending_functions{false};

  ::hare::Ptr<Reactor> reactor{nullptr};
  CycleNotify notify_event{};
  std::atomic<Event::Id> event_id{1};

  mutable std::mutex functions_mutex{};
  std::list<Task> pending_functions{};

  std::uint64_t cycle_index{0};
)
// clang-format on

Cycle::Cycle(REACTOR_TYPE _type) : impl_(new CycleImpl) {
  IMPL->tid = ThreadStoreData().tid;
  IMPL->reactor.reset(CHECK_NULL(Reactor::CreateByType(_type)));

  if (ThreadStoreData().cycle != nullptr) {
    HARE_INTERNAL_FATAL("another cycle[{}] exists in this thread[{:#x}]",
                        (void*)ThreadStoreData().cycle, ThreadStoreData().tid);
  } else {
    ThreadStoreData().cycle = this;
    HARE_INTERNAL_TRACE("cycle[{}] is being initialized in thread[{:#x}].",
                        (void*)this, ThreadStoreData().tid);
  }
}

Cycle::~Cycle() {
  // clear thread local data.
  AssertInCycleThread();
  IMPL->pending_functions.clear();
  ThreadStoreData().cycle = nullptr;
  delete impl_;
}

auto Cycle::ReactorReturnTime() const -> Timestamp {
  return IMPL->reactor_time;
}

auto Cycle::EventHandling() const -> bool {
  return IMPL->calling_pending_functions;
}

auto Cycle::is_running() const -> bool { return IMPL->is_running; }

auto Cycle::type() const -> REACTOR_TYPE { return IMPL->reactor->type(); }

#ifdef HARE_DEBUG

auto Cycle::cycle_index() const -> std::uint64_t { return IMPL->cycle_index; }

#endif

auto Cycle::InCycleThread() const -> bool {
  return IMPL->tid == ThreadStoreData().tid;
}

void Cycle::Exec() {
  HARE_ASSERT(!IMPL->is_running);
  IMPL->is_running = true;
  IMPL->quit = false;
  EventUpdate(&(IMPL->notify_event));

  HARE_INTERNAL_TRACE("cycle[{}] start running...", (void*)this);

  while (!IMPL->quit) {
    EventsList active_events{};

    IMPL->reactor_time = IMPL->reactor->Poll(
        cycle_inner::GetWaitTime(IMPL->reactor->ptimer_), active_events);

#ifdef HARE_DEBUG
    ++IMPL->cycle_index;
#endif

    /// TODO(l1ang70): sort event by priority
    IMPL->event_handling = true;

    for (auto& event_elem : active_events) {
      auto* current_active_event = event_elem.event;
      if (EventCheck(current_active_event)) {
        cycle_inner::PrintActiveEvents(current_active_event);
        current_active_event->HandleEvent(event_elem.revents,
                                          IMPL->reactor_time);
        if (CHECK_EVENT(current_active_event->events(), EVENT_PERSIST) == 0) {
          EventRemove(current_active_event);
        }
      }
    }

    IMPL->event_handling = false;

    NotifyTimer();
    DoPendingFunctions();
  }

  IMPL->notify_event.Deactivate();
  IMPL->is_running = false;

  for (const auto& event : IMPL->reactor->events_) {
    event.second.event->Reset();
  }
  IMPL->reactor->events_.clear();
  PriorityTimer().swap(IMPL->reactor->ptimer_);

  HARE_INTERNAL_TRACE("cycle[{}] stop running...", (void*)this);
}

void Cycle::Exit() {
  IMPL->quit = true;

  /**
   * @brief There is a chance that `Exit()` just executes while(!quit_) and
   * exits, then Cycle destructs, then we are accessing an invalid object.
   */
  if (!InCycleThread()) {
    Notify();
  }
}

void Cycle::RunInCycle(Task _task) {
  if (InCycleThread()) {
    _task();
  } else {
    QueueInCycle(std::move(_task));
  }
}

void Cycle::QueueInCycle(Task _task) {
  {
    std::lock_guard<std::mutex> guard(IMPL->functions_mutex);
    IMPL->pending_functions.push_back(std::move(_task));
  }

  if (!InCycleThread()) {
    Notify();
  }
}

auto Cycle::QueueSize() const -> std::size_t {
  std::lock_guard<std::mutex> guard(IMPL->functions_mutex);
  return IMPL->pending_functions.size();
}

auto Cycle::RunAfter(const Task& _task, std::int64_t _delay) -> Event::Id {
  if (!is_running()) {
    return -1;
  }

  Event* timer = new Timer(_task, _delay);
  auto id = IMPL->event_id.fetch_add(1);

  RunInCycle([timer, id, this] {
    HARE_ASSERT(timer->id() == 0);

    timer->Active(this, id);

    IMPL->reactor->AddEvent(timer, true);
  });

  return id;
}

auto Cycle::RunEvery(const Task& _task, std::int64_t _delay) -> Event::Id {
  if (!is_running()) {
    return -1;
  }

  Event* timer = new Timer(_task, _delay, true);
  auto id = IMPL->event_id.fetch_add(1);

  RunInCycle([this, timer, id] {
    HARE_ASSERT(timer->id() == 0);

    timer->Active(this, id);

    IMPL->reactor->AddEvent(timer, true);
  });

  return id;
}

void Cycle::Cancel(Event::Id _event_id) {
  if (!is_running()) {
    return;
  }

  CountDownLatch cdl{1};

  RunInCycle([&] {
    auto iter = IMPL->reactor->events_.find(_event_id);
    if (iter != IMPL->reactor->events_.end()) {
      auto event_elem = iter->second;
      if (event_elem.event->fd() < 0) {
        event_elem.event->Reset();
        IMPL->reactor->events_.erase(iter);
      } else {
        HARE_INTERNAL_ERROR(
            "cannot \'Cancel\' an event with non-zero file descriptors.");
      }
    } else {
      HARE_INTERNAL_TRACE("event[{}] already finished/cancelled!", _event_id);
    }
    cdl.CountDown();
  });

  if (!InCycleThread()) {
    cdl.Await();
  }
}

void Cycle::EventUpdate(Event* _event) {
  if (_event->cycle() != this && _event->id() != 0) {
    HARE_INTERNAL_ERROR("cannot add event from other cycle[{}].",
                        (void*)_event->cycle());
    return;
  }

  CountDownLatch cdl{1};

  RunInCycle([&cdl, _event, this] {
    if (_event->id() == 0) {
      _event->Active(this, IMPL->event_id.fetch_add(1));
    }

    if (_event->fd() >= 0 && !IMPL->reactor->EventUpdate(_event)) {
      cdl.CountDown();
      return;
    }

    IMPL->reactor->AddEvent(_event);

    cdl.CountDown();
  });

  if (!InCycleThread()) {
    cdl.Await();
  }
}

void Cycle::EventRemove(Event* _event) {
  if (_event->cycle() != this || _event->id() == 0) {
    HARE_INTERNAL_ERROR("the event is not part of the cycle[{}].",
                        (void*)_event->cycle());
    return;
  }

  CountDownLatch cdl{1};

  RunInCycle([&cdl, _event, this] {
    if (!EventCheck(_event)) {
      HARE_INTERNAL_ERROR("event is empty before it was released.");
      cdl.CountDown();
      return;
    }
    auto event_id = _event->id();
    auto fd = _event->fd();

    auto iter = IMPL->reactor->events_.find(event_id);
    if (iter == IMPL->reactor->events_.end()) {
      HARE_INTERNAL_ERROR("cannot find event in cycle[{}]", (void*)this);
    } else {
      HARE_ASSERT(iter->second.event == _event);

      _event->Reset();

      if (fd >= 0) {
        IMPL->reactor->EventRemove(_event);
      }

      IMPL->reactor->events_.erase(iter);
    }

    cdl.CountDown();
  });

  if (!InCycleThread()) {
    cdl.Await();
  }
}

auto Cycle::EventCheck(const Event* _event) const -> bool {
  if (_event->id() < 0) {
    return false;
  }
  AssertInCycleThread();
  return IMPL->reactor->Check(_event);
}

void Cycle::Notify() { IMPL->notify_event.SendNotify(); }

void Cycle::AbortNotCycleThread() const {
  HARE_INTERNAL_FATAL(
      "cycle[{}] was created in thread[{:#x}], current thread is: {:#x}",
      (void*)this, IMPL->tid, ThreadStoreData().tid);
}

void Cycle::DoPendingFunctions() {
  std::list<Task> functions{};
  IMPL->calling_pending_functions = true;

  {
    std::lock_guard<std::mutex> guard(IMPL->functions_mutex);
    functions.swap(IMPL->pending_functions);
  }

  for (const auto& function : functions) {
    function();
  }

  IMPL->calling_pending_functions = false;
}

void Cycle::NotifyTimer() {
  auto& priority_event = IMPL->reactor->ptimer_;
  auto& events = IMPL->reactor->events_;
  auto now = Timestamp::Now();

  while (!priority_event.empty()) {
    const auto& top = priority_event.top();
    if (IMPL->reactor_time < top.stamp) {
      break;
    }
    auto iter = events.find(top.id);
    if (iter != events.end()) {
      auto* event = iter->second.event;
      HARE_INTERNAL_TRACE("event[{}] trigged.", (void*)event);
      event->HandleEvent(EVENT_TIMEOUT, now);
      if (CHECK_EVENT(event->events(), EVENT_PERSIST) != 0) {
        priority_event.emplace(
            event->id(),
            Timestamp(IMPL->reactor_time.microseconds_since_epoch() +
                      event->timeval()));
      } else {
        EventRemove(event);
      }
    } else {
      HARE_INTERNAL_TRACE("event[{}] deleted.", top.id);
    }
    priority_event.pop();
  }
}

}  // namespace hare
