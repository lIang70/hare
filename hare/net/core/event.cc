#include <hare/net/core/event.h>

#include "hare/net/core/cycle.h"
#include <hare/base/logging.h>

#include <sstream>

namespace hare {
namespace core {

    namespace detail {
        auto flagsToString(util_socket_t fd, int32_t event_flags_) -> std::string
        {
            std::ostringstream oss {};
            oss << fd << ": ";
            if (event_flags_ == EVENT_DEFAULT) {
                oss << "DEFAULT";
            } else {
                if ((event_flags_ & EVENT_READ) != 0) {
                    oss << "READ ";
                }
                if ((event_flags_ & EVENT_WRITE) != 0) {
                    oss << "WRITE ";
                }
                if ((event_flags_ & EVENT_ET) != 0) {
                    oss << "ET ";
                }
                if ((event_flags_ & EVENT_CLOSED) != 0) {
                    oss << "CLOSED ";
                }
                if ((event_flags_ & EVENT_ERROR) != 0) {
                    oss << "ERROR ";
                }
            }
            return oss.str();
        }
    } // namespace detail

    Event::Event(Cycle* cycle, util_socket_t target_fd)
        : owner_cycle_(cycle)
        , fd_(target_fd)
    {
    }

    Event::~Event()
    {
        HARE_ASSERT(!event_handle_, "When the event is destroyed, the event is still handling.");
        HARE_ASSERT(!added_to_cycle_, "When the event is destroyed, the event is still in the cycle.");
        if (owner_cycle_ != nullptr) {
            HARE_ASSERT(!owner_cycle_->checkEvent(this), "When the event is destroyed, the event is still held by the cycle.");
        }
    }

    auto Event::flagsToString() const -> std::string
    {
        return detail::flagsToString(fd_, event_flags_);
    }

    auto Event::rflagsToString() const -> std::string
    {
        return detail::flagsToString(fd_, revent_flags_);
    }

    void Event::handleEvent(Timestamp& receive_time)
    {
        std::shared_ptr<void> object;
        if (tied_) {
            object = tie_object_.lock();
            if (!object) {
                return;
            }
        }
        event_handle_.exchange(true);
        eventCallBack(revent_flags_, receive_time);
        event_handle_.exchange(false);
    }

    void Event::tie(const std::shared_ptr<void>& object)
    {
        tie_object_ = object;
        tied_.exchange(true);
    }

    void Event::deactive()
    {
        HARE_ASSERT(isNoneEvent(), "Event is not correct.");
        if (owner_cycle_ != nullptr) {
            owner_cycle_->removeEvent(this);
            added_to_cycle_.exchange(false);
        }
    }

    void Event::active()
    {
        if (owner_cycle_ != nullptr) {
            added_to_cycle_.exchange(true);
            owner_cycle_->updateEvent(this);
        }
    }

} // namespace core
} // namespace hare
