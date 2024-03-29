#include <hare/base/io/event.h>

#include <sstream>

#include "base/fwd-inl.h"
#include "base/io/reactor.h"

namespace hare {
namespace io {

namespace detail {

static auto EventsToString(util_socket_t _fd, std::uint8_t _events)
    -> std::string {
  std::stringstream oss{};
  oss << _fd << ": ";
  if (_events == EVENT_DEFAULT) {
    oss << "DEFAULT";
    return oss.str();
  }
  if (CHECK_EVENT(_events, EVENT_TIMEOUT) != 0) {
    oss << "TIMEOUT ";
  }
  if (CHECK_EVENT(_events, EVENT_READ) != 0) {
    oss << "READ ";
  }
  if (CHECK_EVENT(_events, EVENT_WRITE) != 0) {
    oss << "WRITE ";
  }
  if (CHECK_EVENT(_events, EVENT_PERSIST) != 0) {
    oss << "PERSIST ";
  }
  if (CHECK_EVENT(_events, EVENT_ET) != 0) {
    oss << "ET ";
  }
  if (CHECK_EVENT(_events, EVENT_CLOSED) != 0) {
    oss << "CLOSED ";
  }
  return oss.str();
}
}  // namespace detail

HARE_IMPL_DEFAULT(Event, util_socket_t fd{-1};
                  std::uint8_t events{EVENT_DEFAULT};
                  Event::Callback callback{}; std::int64_t timeval{0};

                  Cycle * cycle{}; Event::Id id{-1}; std::int64_t timeout{0};

                  bool tied{false}; WPtr<void> tie_object{};)

Event::Event(util_socket_t _fd, Callback _cb, std::uint8_t _events,
             std::int64_t _timeval)
    : impl_(new EventImpl) {
  IMPL->fd = _fd;
  IMPL->events = _events;
  IMPL->callback = std::move(_cb);
  IMPL->timeval = _timeval;
  if (CHECK_EVENT(IMPL->events, EVENT_TIMEOUT) != 0 &&
      CHECK_EVENT(IMPL->events, EVENT_PERSIST) != 0) {
    CLEAR_EVENT(IMPL->events, EVENT_TIMEOUT);
    IMPL->timeval = 0;
    HARE_INTERNAL_ERROR(
        "cannot be set EVENT_PERSIST and EVENT_TIMEOUT at the same time.");
  }
}

Event::~Event() {
  HARE_ASSERT(IMPL->cycle == nullptr);
  delete impl_;
}

auto Event::fd() const -> util_socket_t { return IMPL->fd; }

auto Event::events() const -> std::uint8_t { return IMPL->events; }

auto Event::timeval() const -> std::int64_t { return IMPL->timeval; }

auto Event::cycle() const -> Cycle* { return IMPL->cycle; }

auto Event::id() const -> Id { return IMPL->id; }

void Event::EnableRead() {
  SET_EVENT(IMPL->events, EVENT_READ);
  if (IMPL->cycle) {
    IMPL->cycle->EventUpdate(shared_from_this());
  } else {
    HARE_INTERNAL_ERROR("event[{}] need to be added to cycle.", (void*)this);
  }
}

void Event::DisableRead() {
  CLEAR_EVENT(IMPL->events, EVENT_READ);
  if (IMPL->cycle) {
    IMPL->cycle->EventUpdate(shared_from_this());
  } else {
    HARE_INTERNAL_ERROR("event[{}] need to be added to cycle.", (void*)this);
  }
}

auto Event::Reading() -> bool {
  return CHECK_EVENT(IMPL->events, EVENT_READ) != 0;
}

void Event::EnableWrite() {
  SET_EVENT(IMPL->events, EVENT_WRITE);
  if (IMPL->cycle) {
    IMPL->cycle->EventUpdate(shared_from_this());
  } else {
    HARE_INTERNAL_ERROR("event[{}] need to be added to cycle.", (void*)this);
  }
}

void Event::DisableWrite() {
  CLEAR_EVENT(IMPL->events, EVENT_WRITE);
  if (IMPL->cycle) {
    IMPL->cycle->EventUpdate(shared_from_this());
  } else {
    HARE_INTERNAL_ERROR("event[{}] need to be added to cycle.", (void*)this);
  }
}

auto Event::Writing() -> bool {
  return CHECK_EVENT(IMPL->events, EVENT_WRITE) != 0;
}

void Event::Deactivate() {
  if (IMPL->cycle != nullptr) {
    IMPL->cycle->EventRemove(shared_from_this());
  } else {
    HARE_INTERNAL_ERROR("event[{}] need to be added to cycle.", (void*)this);
  }
}

auto Event::EventToString() const -> std::string {
  return detail::EventsToString(IMPL->fd, IMPL->events);
}

void Event::Tie(const hare::Ptr<void>& _obj) {
  IMPL->tie_object = _obj;
  IMPL->tied = true;
}

auto Event::TiedObject() -> WPtr<void> { return IMPL->tie_object; }

void Event::HandleEvent(std::uint8_t _flag, Timestamp& _receive_time) {
  hare::Ptr<void> object;
  if (IMPL->tied) {
    object = IMPL->tie_object.lock();
    if (!object) {
      return;
    }
    if (IMPL->callback) {
      IMPL->callback(shared_from_this(), _flag, _receive_time);
    }
  }
}

void Event::Active(Cycle* _cycle, Event::Id _id) {
  IMPL->cycle = CHECK_NULL(_cycle);
  IMPL->id = _id;
}

void Event::Reset() {
  IMPL->cycle = nullptr;
  IMPL->id = -1;
}

}  // namespace io
}  // namespace hare
