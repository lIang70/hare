#ifndef _HARE_BASE_IO_REACTOR_H_
#define _HARE_BASE_IO_REACTOR_H_

#include <hare/base/io/cycle.h>

#include <list>
#include <map>
#include <queue>

#define POLL_TIME_MICROSECONDS 1000000

#define CHECK_EVENT(event, val) ((event) & (val))
#define SET_EVENT(event, val) ((event) |= (val))
#define CLEAR_EVENT(event, val) ((event) &= ~(val))

namespace hare {

namespace io_inner {

struct EventElem {
  Event::Id id{};
  Event* event{};
  std::uint8_t revents{EVENT_DEFAULT};
  bool need_free{false};

  EventElem(Event::Id _id, Event* _event, std::uint8_t _revents,
            bool _need_free = false)
      : id(_id), event(_event), revents(_revents), need_free(_need_free) {}

  ~EventElem() {
    if (need_free) {
      delete event;
    }
  }
};

struct TimerElem {
  Event::Id id{};
  Timestamp stamp{};
  TimerElem(Event::Id _id, Timestamp _stamp) : id(_id), stamp(_stamp) {}
};

struct TimerPriority {
  auto operator()(const TimerElem& _elem_x, const TimerElem& _elem_y) -> bool {
    return _elem_x.stamp < _elem_y.stamp;
  }
};

}  // namespace io_inner

using EventsList = std::list<io_inner::EventElem>;
using EventsMap = std::map<Event::Id, io_inner::EventElem>;
using PriorityTimer =
    std::priority_queue<io_inner::TimerElem, std::vector<io_inner::TimerElem>,
                        io_inner::TimerPriority>;

class Reactor : public NonCopyable {
  Cycle::REACTOR_TYPE type_{};

 protected:
  EventsMap events_{};
  PriorityTimer ptimer_{};

 public:
  static auto CreateByType(Cycle::REACTOR_TYPE _type) -> Reactor*;

  virtual ~Reactor() = default;

  HARE_INLINE
  auto type() -> Cycle::REACTOR_TYPE { return type_; }

  HARE_INLINE
  auto Check(const Event* _event) const -> bool {
    auto iter = events_.find(_event->id());
    return iter != events_.end() && iter->second.event == _event;
  }

  void AddEvent(Event* event, bool need_free = false);

  /**
   * @brief Polls the I/O events.
   *   Must be called in the cycle thread.
   */
  virtual auto Poll(std::int32_t _timeout_microseconds, EventsList& _events)
      -> Timestamp = 0;

  /**
   *  @brief Changes the interested I/O events.
   *   Must be called in the cycle thread.
   */
  virtual auto EventUpdate(Event* _event) -> bool = 0;

  /**
   *  @brief Remove the event, when it destructs.
   *   Must be called in the cycle thread.
   */
  virtual auto EventRemove(Event* _event) -> bool = 0;

 protected:
  explicit Reactor(Cycle::REACTOR_TYPE _type);

  friend class Cycle;
};

}  // namespace hare

#endif  // _HARE_BASE_IO_REACTOR_H_