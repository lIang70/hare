#include "hare/base/fwd-inl.h"
#include "hare/base/io/reactor.h"
#include <hare/base/io/event.h>

#include <sstream>

namespace hare {
namespace io {

    namespace detail {

        auto EventsToString(util_socket_t _fd, std::uint8_t _events) -> std::string
        {
            std::stringstream oss {};
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
    } // namespace detail

    HARE_IMPL_DEFAULT(
        Event,
        util_socket_t fd { -1 };
        std::uint8_t events { EVENT_DEFAULT };
        Event::Callback callback {};
        std::int64_t timeval { 0 };

        Cycle* cycle {};
        Event::Id id { -1 };
        std::int64_t timeout { 0 };

        bool tied { false };
        WPtr<void> tie_object {};
    )

    Event::Event(util_socket_t _fd, Callback _cb, std::uint8_t _events, std::int64_t _timeval)
        : impl_(new EventImpl)
    {
        d_ptr(impl_)->fd = _fd;
        d_ptr(impl_)->events = _events;
        d_ptr(impl_)->callback = std::move(_cb);
        d_ptr(impl_)->timeval = _timeval;
        if (CHECK_EVENT(d_ptr(impl_)->events, EVENT_TIMEOUT) != 0 && CHECK_EVENT(d_ptr(impl_)->events, EVENT_PERSIST) != 0) {
            CLEAR_EVENT(d_ptr(impl_)->events, EVENT_TIMEOUT);
            d_ptr(impl_)->timeval = 0;
            MSG_ERROR("cannot be set EVENT_PERSIST and EVENT_TIMEOUT at the same time.");
        }
    }

    Event::~Event()
    {
        assert(d_ptr(impl_)->cycle == nullptr);
        delete impl_;
    }

    auto Event::fd() const -> util_socket_t
    {
        return d_ptr(impl_)->fd;
    }

    auto Event::events() const -> std::uint8_t
    {
        return d_ptr(impl_)->events;
    }

    auto Event::timeval() const -> std::int64_t
    {
        return d_ptr(impl_)->timeval;
    }

    auto Event::cycle() const -> Cycle*
    {
        return d_ptr(impl_)->cycle;
    }

    auto Event::id() const -> Id
    {
        return d_ptr(impl_)->id;
    }

    void Event::EnableRead()
    {
        SET_EVENT(d_ptr(impl_)->events, EVENT_READ);
        if (d_ptr(impl_)->cycle) {
            d_ptr(impl_)->cycle->EventUpdate(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    void Event::DisableRead()
    {
        CLEAR_EVENT(d_ptr(impl_)->events, EVENT_READ);
        if (d_ptr(impl_)->cycle) {
            d_ptr(impl_)->cycle->EventUpdate(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    auto Event::Reading() -> bool
    {
        return CHECK_EVENT(d_ptr(impl_)->events, EVENT_READ) != 0;
    }

    void Event::EnableWrite()
    {
        SET_EVENT(d_ptr(impl_)->events, EVENT_WRITE);
        if (d_ptr(impl_)->cycle) {
            d_ptr(impl_)->cycle->EventUpdate(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    void Event::DisableWrite()
    {
        CLEAR_EVENT(d_ptr(impl_)->events, EVENT_WRITE);
        if (d_ptr(impl_)->cycle) {
            d_ptr(impl_)->cycle->EventUpdate(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    auto Event::Writing() -> bool
    {
        return CHECK_EVENT(d_ptr(impl_)->events, EVENT_WRITE) != 0;
    }

    void Event::Deactivate()
    {
        if (d_ptr(impl_)->cycle != nullptr) {
            d_ptr(impl_)->cycle->EventRemove(shared_from_this());
        } else {
            MSG_ERROR("event[{}] need to be added to cycle.", (void*)this);
        }
    }

    auto Event::EventToString() const -> std::string
    {
        return detail::EventsToString(d_ptr(impl_)->fd, d_ptr(impl_)->events);
    }

    void Event::Tie(const hare::Ptr<void>& _obj)
    {
        d_ptr(impl_)->tie_object = _obj;
        d_ptr(impl_)->tied = true;
    }

    auto Event::TiedObject() -> WPtr<void>
    {
        return d_ptr(impl_)->tie_object;
    }

    void Event::HandleEvent(std::uint8_t _flag, Timestamp& _receive_time)
    {
        hare::Ptr<void> object;
        if (d_ptr(impl_)->tied) {
            object = d_ptr(impl_)->tie_object.lock();
            if (!object) {
                return;
            }
            if (d_ptr(impl_)->callback) {
                d_ptr(impl_)->callback(shared_from_this(), _flag, _receive_time);
            }
        }
    }

    void Event::Active(Cycle* _cycle, Event::Id _id)
    {
        d_ptr(impl_)->cycle = CHECK_NULL(_cycle);
        d_ptr(impl_)->id = _id;
    }

    void Event::Reset()
    {
        d_ptr(impl_)->cycle = nullptr;
        d_ptr(impl_)->id = -1;
    }

} // namespace io
} // namespace hare
