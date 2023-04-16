#include <hare/net/session.h>

#include "hare/base/io/reactor.h"
#include "hare/net/socket_op.h"
#include <hare/base/logging.h>

namespace hare {
namespace net {

    namespace detail {

        auto state_to_string(STATE state) -> const char*
        {
            switch (state) {
            case STATE_CONNECTING:
                return "STATE_CONNECTING";
            case STATE_CONNECTED:
                return "STATE_CONNECTED";
            case STATE_DISCONNECTING:
                return "STATE_DISCONNECTING";
            case STATE_DISCONNECTED:
                return "STATE_DISCONNECTED";
            default:
                return "UNKNOWN STATE";
            }
        }
    } // namespace detail

    session::~session()
    {
        HARE_ASSERT(state_ == STATE_DISCONNECTED, "session should be disconnected before free.");
        LOG_TRACE() << "session[" << name_ << "] at " << this
                    << " fd: " << fd() << " free.";
    }

    auto session::shutdown() -> error
    {
        if (state_ == STATE_CONNECTED) {
            set_state(STATE_DISCONNECTING);
            if (CHECK_EVENT(event_->events(), io::EVENT_WRITE) == 0) {
                return socket_.shutdown_write();
            }
            return error(HARE_ERROR_SOCKET_WRITING);
        }
        return error(HARE_ERROR_SESSION_ALREADY_DISCONNECT);
    }

    auto session::force_close() -> error
    {
        if (state_ == STATE_CONNECTED || state_ == STATE_DISCONNECTING) {
            set_state(STATE_DISCONNECTING);
            handle_close();
        }
        return error(HARE_ERROR_SESSION_ALREADY_DISCONNECT);
    }

    void session::start_read()
    {
        if (!reading_ || CHECK_EVENT(event_->events(), io::EVENT_READ) == 0) {
            event_->enable_read();
            reading_ = true;
        }
    }

    void session::stop_read()
    {
        if (reading_ || CHECK_EVENT(event_->events(), io::EVENT_READ) != 0) {
            event_->disable_read();
            reading_ = false;
        }
    }

    session::session(io::cycle* _cycle, TYPE _type,
        host_address _local_addr,
        std::string _name, int8_t _family, util_socket_t _fd,
        host_address _peer_addr)
        : cycle_(HARE_CHECK_NULL(_cycle))
        , event_(new io::event(_fd,
              std::bind(&session::handle_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
              SESSION_READ | SESSION_WRITE | io::EVENT_PERSIST,
              0))
        , socket_(_family, _type, _fd)
        , local_addr_(std::move(_local_addr))
        , peer_addr_(std::move(_peer_addr))
        , name_(std::move(_name))
    {
    }

    void session::handle_callback(const io::event::ptr& _event, uint8_t _events, const timestamp& _receive_time)
    {
        owner_cycle()->assert_in_cycle_thread();
        HARE_ASSERT(_event == event_, "error event.");
        LOG_TRACE() << "session[" << event_->fd() << "] revents: " << _events;
        if (CHECK_EVENT(_events, SESSION_READ)) {
            handle_read(_receive_time);
        }
        if (CHECK_EVENT(_events, SESSION_WRITE)) {
            handle_write();
        }
        if (CHECK_EVENT(_events, SESSION_CLOSED)) {
            handle_close();
        }
    }

    void session::handle_close()
    {
        LOG_TRACE() << "fd = " << fd() << " state = " << detail::state_to_string(state_);
        HARE_ASSERT(state_ == STATE_CONNECTED || state_ == STATE_DISCONNECTING, "");
        // we don't close fd, leave it to dtor, so we can find leaks easily.
        set_state(STATE_DISCONNECTED);
        event_->disable_read();
        event_->disable_write();
        if (connect_) {
            connect_(shared_from_this(), SESSION_CLOSED);
        } else {
            SYS_ERROR() << "cannot set connect_callback to session[" << name() << "], session is closed.";
        }
    }

    void session::handle_error()
    {
        if (connect_) {
            connect_(shared_from_this(), SESSION_ERROR);
        } else {
            SYS_ERROR() << "occurred error to the session[" << name_ << "], detail: " << socket_op::get_socket_error_info(fd());
        }
    }

    void session::connect_established()
    {
        HARE_ASSERT(state_ == STATE_CONNECTING, "");
        set_state(STATE_CONNECTED);
        if (connect_) {
            connect_(shared_from_this(), SESSION_CONNECTED);
        } else {
            SYS_ERROR() << "cannot set connect_callback to session[" << name() << "], session connected.";
        }
        event_->tie(shared_from_this());
        cycle_->event_update(event_);
        event_->enable_read();
    }

    void session::connect_destroyed()
    {
        if (state_ == STATE_CONNECTED) {
            handle_close();
        }
        event_->deactivate();
    }

} // namespace net
} // namespace hare