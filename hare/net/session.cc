#include "hare/base/fwd-inl.h"
#include "hare/base/io/reactor.h"
#include "hare/net/socket_op.h"
#include <hare/net/session.h>

#include <cassert>

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
        assert(state_ == STATE_DISCONNECTED);
        MSG_TRACE("session[{}] at {} fd={} free.", name_, (void*)this, fd());
    }

    auto session::shutdown() -> error
    {
        if (state_ == STATE_CONNECTED) {
            set_state(STATE_DISCONNECTING);
            if (!event_->writing()) {
                return socket_.shutdown_write();
            }
            return error(ERROR_SOCKET_WRITING);
        }
        return error(ERROR_SESSION_ALREADY_DISCONNECT);
    }

    auto session::force_close() -> error
    {
        if (state_ == STATE_CONNECTED || state_ == STATE_DISCONNECTING) {
            handle_close();
            return error(ERROR_SUCCESS);
        }
        return error(ERROR_SESSION_ALREADY_DISCONNECT);
    }

    void session::start_read()
    {
        if (!reading_ || !event_->reading()) {
            event_->enable_read();
            reading_ = true;
        }
    }

    void session::stop_read()
    {
        if (reading_ || event_->reading()) {
            event_->disable_read();
            reading_ = false;
        }
    }

    session::session(io::cycle* _cycle, TYPE _type,
        host_address _local_addr,
        std::string _name, int8_t _family, util_socket_t _fd,
        host_address _peer_addr)
        : cycle_(CHECK_NULL(_cycle))
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
        assert(_event == event_);
        MSG_TRACE("session[{}] revents: {}.", event_->fd(), _events);
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
        MSG_TRACE("fd={} state={}.", fd(), detail::state_to_string(state_));
        assert(state_ == STATE_CONNECTED || state_ == STATE_DISCONNECTING);
        // we don't close fd, leave it to dtor, so we can find leaks easily.
        set_state(STATE_DISCONNECTED);
        event_->disable_read();
        event_->disable_write();
        if (connect_) {
            connect_(shared_from_this(), SESSION_CLOSED);
        } else {
            MSG_ERROR("connect_callback has not been set for session[{}], session is closed.", name_);
        }
        event_->deactivate();
        destroy_();
    }

    void session::handle_error()
    {
        if (connect_) {
            connect_(shared_from_this(), SESSION_ERROR);
        } else {
            MSG_ERROR("occurred error to the session[{}], detail: {}.",
                name_, socket_op::get_socket_error_info(fd()));
        }
    }

    void session::connect_established()
    {
        assert(state_ == STATE_CONNECTING);
        set_state(STATE_CONNECTED);
        if (connect_) {
            connect_(shared_from_this(), SESSION_CONNECTED);
        } else {
            MSG_ERROR("connect_callback has not been set for session[{}], session connected.", name_);
        }
        event_->tie(shared_from_this());
        cycle_->event_update(event_);
        event_->enable_read();
    }

} // namespace net
} // namespace hare