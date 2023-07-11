#include "hare/base/io/local.h"
#include "hare/base/io/reactor.h"
#include <hare/base/io/event.h>

#include <sstream>

namespace hare {
namespace io {

    namespace detail {

        auto to_string(util_socket_t fd, std::uint8_t flag) -> std::string
        {
            std::stringstream oss {};
            oss << fd << ": ";
            if (flag == EVENT_DEFAULT) {
                oss << "DEFAULT";
                return oss.str();
            }
            if (CHECK_EVENT(flag, EVENT_TIMEOUT) != 0) {
                oss << "TIMEOUT ";
            }
            if (CHECK_EVENT(flag, EVENT_READ) != 0) {
                oss << "READ ";
            }
            if (CHECK_EVENT(flag, EVENT_WRITE) != 0) {
                oss << "WRITE ";
            }
            if (CHECK_EVENT(flag, EVENT_PERSIST) != 0) {
                oss << "PERSIST ";
            }
            if (CHECK_EVENT(flag, EVENT_ET) != 0) {
                oss << "ET ";
            }
            if (CHECK_EVENT(flag, EVENT_CLOSED) != 0) {
                oss << "CLOSED ";
            }
            return oss.str();
        }
    } // namespace detail

    event::event(util_socket_t _fd, callback _cb, std::uint8_t _flag, std::int64_t _timeval)
        : fd_(_fd)
        , events_(_flag)
        , callback_(std::move(_cb))
        , timeval_(_timeval)
    {
        if (CHECK_EVENT(events_, EVENT_TIMEOUT) != 0 && CHECK_EVENT(events_, EVENT_PERSIST) != 0) {
            CLEAR_EVENT(events_, EVENT_TIMEOUT);
            timeval_ = 0;
            MSG_ERROR("cannot be set EVENT_PERSIST and EVENT_TIMEOUT at the same time.");
        }
    }

    event::~event()
    {
        assert(cycle_ == nullptr);
    }

    void event::enable_read()
    {
        SET_EVENT(events_, EVENT_READ);
        if (cycle_) {
            cycle_->event_update(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    void event::disable_read()
    {
        CLEAR_EVENT(events_, EVENT_READ);
        if (cycle_) {
            cycle_->event_update(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    auto event::reading() -> bool
    {
        return CHECK_EVENT(events_, EVENT_READ) != 0;
    }

    void event::enable_write()
    {
        SET_EVENT(events_, EVENT_WRITE);
        if (cycle_) {
            cycle_->event_update(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    void event::disable_write()
    {
        CLEAR_EVENT(events_, EVENT_WRITE);
        if (cycle_) {
            cycle_->event_update(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    auto event::writing() -> bool
    {
        return CHECK_EVENT(events_, EVENT_WRITE) != 0;
    }

    void event::deactivate()
    {
        cycle_->event_remove(shared_from_this());
    }

    auto event::event_string() const -> std::string
    {
        return detail::to_string(fd_, events_);
    }

    void event::tie(const hare::ptr<void>& _obj)
    {
        tie_object_ = _obj;
        tied_ = true;
    }

    void event::handle_event(std::uint8_t _flag, timestamp& _receive_time)
    {
        hare::ptr<void> object;
        if (tied_) {
            object = tie_object_.lock();
            if (!object) {
                return;
            }
            if (callback_) {
                callback_(shared_from_this(), _flag, _receive_time);
            }
        }
    }

} // namespace io
} // namespace hare
