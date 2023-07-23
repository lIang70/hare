#include "hare/base/fwd-inl.h"
#include "hare/base/io/reactor.h"
#include <hare/base/io/socket_op.h>
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

    HARE_IMPL_DEFAULT(session,
        std::string name_ {};
        io::cycle * cycle_ { nullptr };
        io::event::ptr event_ { nullptr };
        socket socket_;

        const host_address local_addr_ {};
        const host_address peer_addr_ {};

        bool reading_ { false };
        STATE state_ { STATE_CONNECTING };

        session::connect_callback connect_ {};
        session::destroy destroy_ {};

        util::any any_ctx_ {};

        session_impl(host_address _local_addr,
            std::uint8_t _family, TYPE _type, util_socket_t _fd,
            host_address _peer_addr)
            : socket_(_family, _type, _fd)
            , local_addr_(std::move(_local_addr))
            , peer_addr_(std::move(_peer_addr))
        {}
    )

    session::~session()
    {
        assert(d_ptr(impl_)->state_ == STATE_DISCONNECTED);
        MSG_TRACE("session[{}] at {} fd={} free.", d_ptr(impl_)->name_, (void*)this, fd());
        delete impl_;
    }

    auto session::name() const -> std::string 
    { return d_ptr(impl_)->name_; }
    auto session::owner_cycle() const -> io::cycle* 
    { return d_ptr(impl_)->cycle_; }
    auto session::local_address() const -> const host_address& 
    { return d_ptr(impl_)->local_addr_; }
    auto session::peer_address() const -> const host_address& 
    { return d_ptr(impl_)->peer_addr_; }
    auto session::state() const -> STATE 
    { return d_ptr(impl_)->state_; }
    auto session::fd() const -> util_socket_t 
    { return d_ptr(impl_)->socket_.fd(); }

    auto session::connected() const -> bool 
    { return d_ptr(impl_)->state_ == STATE_CONNECTED; }

    void session::set_connect_callback(connect_callback _connect) 
    { d_ptr(impl_)->connect_ = std::move(_connect); }

    void session::set_context(const util::any& context) 
    { d_ptr(impl_)->any_ctx_ = context; }
    auto session::get_context() const -> const util::any& 
    { return d_ptr(impl_)->any_ctx_; }

    auto session::shutdown() -> error
    {
        if (d_ptr(impl_)->state_ == STATE_CONNECTED) {
            set_state(STATE_DISCONNECTING);
            if (!d_ptr(impl_)->event_->writing()) {
                return d_ptr(impl_)->socket_.shutdown_write();
            }
            return error(ERROR_SOCKET_WRITING);
        }
        return error(ERROR_SESSION_ALREADY_DISCONNECT);
    }

    auto session::force_close() -> error
    {
        if (d_ptr(impl_)->state_ == STATE_CONNECTED || d_ptr(impl_)->state_ == STATE_DISCONNECTING) {
            handle_close();
            return error(ERROR_SUCCESS);
        }
        return error(ERROR_SESSION_ALREADY_DISCONNECT);
    }

    void session::start_read()
    {
        if (!d_ptr(impl_)->reading_ || !d_ptr(impl_)->event_->reading()) {
            d_ptr(impl_)->event_->enable_read();
            d_ptr(impl_)->reading_ = true;
        }
    }

    void session::stop_read()
    {
        if (d_ptr(impl_)->reading_ || d_ptr(impl_)->event_->reading()) {
            d_ptr(impl_)->event_->disable_read();
            d_ptr(impl_)->reading_ = false;
        }
    }

    session::session(io::cycle* _cycle, TYPE _type,
        host_address _local_addr,
        std::string _name, std::uint8_t _family, util_socket_t _fd,
        host_address _peer_addr)
        : impl_(new session_impl(std::move(_local_addr), _family, _type, _fd, std::move(_peer_addr)))
    {
        d_ptr(impl_)->cycle_ = CHECK_NULL(_cycle);
        d_ptr(impl_)->event_.reset(new io::event(_fd,
            std::bind(&session::handle_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            SESSION_READ | SESSION_WRITE | io::EVENT_PERSIST,
            0));
        d_ptr(impl_)->name_ = std::move(_name);
    }

    void session::set_state(STATE _state)
    {
        d_ptr(impl_)->state_ = _state;
    }

    auto session::event() -> io::event::ptr&
    {
        return d_ptr(impl_)->event_;
    }

    auto session::socket() -> net::socket&
    {
        return d_ptr(impl_)->socket_;
    }

    void session::set_destroy(destroy _destroy)
    {
        d_ptr(impl_)->destroy_ = std::move(_destroy);
    }

    void session::handle_callback(const io::event::ptr& _event, uint8_t _events, const timestamp& _receive_time)
    {
        owner_cycle()->assert_in_cycle_thread();
        assert(_event == d_ptr(impl_)->event_);
        MSG_TRACE("session[{}] revents: {}.", d_ptr(impl_)->event_->fd(), _events);
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
        MSG_TRACE("fd={} state={}.", fd(), detail::state_to_string(d_ptr(impl_)->state_));
        assert(d_ptr(impl_)->state_ == STATE_CONNECTED || d_ptr(impl_)->state_ == STATE_DISCONNECTING);
        // we don't close fd, leave it to dtor, so we can find leaks easily.
        set_state(STATE_DISCONNECTED);
        d_ptr(impl_)->event_->disable_read();
        d_ptr(impl_)->event_->disable_write();
        if (d_ptr(impl_)->connect_) {
            d_ptr(impl_)->connect_(shared_from_this(), SESSION_CLOSED);
        } else {
            MSG_ERROR("connect_callback has not been set for session[{}], session is closed.", d_ptr(impl_)->name_);
        }
        d_ptr(impl_)->event_->deactivate();
        d_ptr(impl_)->destroy_();
    }

    void session::handle_error()
    {
        if (d_ptr(impl_)->connect_) {
            d_ptr(impl_)->connect_(shared_from_this(), SESSION_ERROR);
        } else {
            MSG_ERROR("occurred error to the session[{}], detail: {}.",
                d_ptr(impl_)->name_, socket_op::get_socket_error_info(fd()));
        }
    }

    void session::connect_established()
    {
        assert(d_ptr(impl_)->state_ == STATE_CONNECTING);
        set_state(STATE_CONNECTED);
        if (d_ptr(impl_)->connect_) {
            d_ptr(impl_)->connect_(shared_from_this(), SESSION_CONNECTED);
        } else {
            MSG_ERROR("connect_callback has not been set for session[{}], session connected.", d_ptr(impl_)->name_);
        }
        d_ptr(impl_)->event_->tie(shared_from_this());
        d_ptr(impl_)->cycle_->event_update(d_ptr(impl_)->event_);
        d_ptr(impl_)->event_->enable_read();
    }

} // namespace net
} // namespace hare