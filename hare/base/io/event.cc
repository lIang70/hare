#include "hare/base/fwd-inl.h"
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

    HARE_IMPL_DEFAULT(event,
        util_socket_t fd_ { -1 };
        uint8_t events_ { EVENT_DEFAULT };
        event::callback callback_ {};
        int64_t timeval_ { 0 };

        cycle* cycle_ {};
        event::id id_ { -1 };
        int64_t timeout_ { 0 };

        bool tied_ { false };
        wptr<void> tie_object_ {};
    )

    event::event(util_socket_t _fd, callback _cb, std::uint8_t _events, std::int64_t _timeval)
        : impl_(new event_impl)
    {
        d_ptr(impl_)->fd_ = _fd;
        d_ptr(impl_)->events_ = _events;
        d_ptr(impl_)->callback_ = std::move(_cb);
        d_ptr(impl_)->timeval_ = _timeval;
        if (CHECK_EVENT(d_ptr(impl_)->events_, EVENT_TIMEOUT) != 0 && CHECK_EVENT(d_ptr(impl_)->events_, EVENT_PERSIST) != 0) {
            CLEAR_EVENT(d_ptr(impl_)->events_, EVENT_TIMEOUT);
            d_ptr(impl_)->timeval_ = 0;
            MSG_ERROR("cannot be set EVENT_PERSIST and EVENT_TIMEOUT at the same time.");
        }
    }

    event::~event()
    {
        assert(d_ptr(impl_)->cycle_ == nullptr);
        delete impl_;
    }

    auto event::fd() const -> util_socket_t
    {
        return d_ptr(impl_)->fd_;
    }

    auto event::events() const -> uint8_t
    {
        return d_ptr(impl_)->events_;
    }

    auto event::timeval() const -> int64_t
    {
        return d_ptr(impl_)->timeval_;
    }

    auto event::owner_cycle() const -> cycle*
    {
        return d_ptr(impl_)->cycle_;
    }

    auto event::event_id() const -> id
    {
        return d_ptr(impl_)->id_;
    }

    void event::enable_read()
    {
        SET_EVENT(d_ptr(impl_)->events_, EVENT_READ);
        if (d_ptr(impl_)->cycle_) {
            d_ptr(impl_)->cycle_->event_update(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    void event::disable_read()
    {
        CLEAR_EVENT(d_ptr(impl_)->events_, EVENT_READ);
        if (d_ptr(impl_)->cycle_) {
            d_ptr(impl_)->cycle_->event_update(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    auto event::reading() -> bool
    {
        return CHECK_EVENT(d_ptr(impl_)->events_, EVENT_READ) != 0;
    }

    void event::enable_write()
    {
        SET_EVENT(d_ptr(impl_)->events_, EVENT_WRITE);
        if (d_ptr(impl_)->cycle_) {
            d_ptr(impl_)->cycle_->event_update(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    void event::disable_write()
    {
        CLEAR_EVENT(d_ptr(impl_)->events_, EVENT_WRITE);
        if (d_ptr(impl_)->cycle_) {
            d_ptr(impl_)->cycle_->event_update(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    auto event::writing() -> bool
    {
        return CHECK_EVENT(d_ptr(impl_)->events_, EVENT_WRITE) != 0;
    }

    void event::deactivate()
    {
        if (d_ptr(impl_)->cycle_ != nullptr) {
            d_ptr(impl_)->cycle_->event_remove(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    auto event::event_string() const -> std::string
    {
        return detail::to_string(d_ptr(impl_)->fd_, d_ptr(impl_)->events_);
    }

    void event::tie(const hare::ptr<void>& _obj)
    {
        d_ptr(impl_)->tie_object_ = _obj;
        d_ptr(impl_)->tied_ = true;
    }

    auto event::tied_object() -> wptr<void>
    {
        return d_ptr(impl_)->tie_object_;
    }

    void event::handle_event(std::uint8_t _flag, timestamp& _receive_time)
    {
        hare::ptr<void> object;
        if (d_ptr(impl_)->tied_) {
            object = d_ptr(impl_)->tie_object_.lock();
            if (!object) {
                return;
            }
            if (d_ptr(impl_)->callback_) {
                d_ptr(impl_)->callback_(shared_from_this(), _flag, _receive_time);
            }
        }
    }

    void event::active(cycle* _cycle, event::id _id)
    {
        d_ptr(impl_)->cycle_ = CHECK_NULL(_cycle);
        d_ptr(impl_)->id_ = _id;
    }

    void event::reset()
    {
        d_ptr(impl_)->cycle_ = nullptr;
        d_ptr(impl_)->id_ = -1;
    }

} // namespace io
} // namespace hare
