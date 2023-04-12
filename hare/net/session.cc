#include "hare/base/error.h"
#include "hare/net/core/event.h"
#include "hare/net/socket.h"
#include "net/socket_op.h"
#include <hare/net/session.h>

#include <hare/base/logging.h>

namespace hare {
namespace net {

    namespace detail {
        auto stateToString(Session::STATE state) -> const char*
        {
            switch (state) {
            case Session::STATE_CONNECTING:
                return "STATE_CONNECTING";
            case Session::STATE_CONNECTED:
                return "STATE_CONNECTED";
            case Session::STATE_DISCONNECTING:
                return "STATE_DISCONNECTING";
            case Session::STATE_DISCONNECTED:
                return "STATE_DISCONNECTED";
            default:
                return "UNKNOWN STATE";
            }
        }
    } // namespace detail

    Session::~Session()
    {
        HARE_ASSERT(state_ == STATE_DISCONNECTED, "");
        LOG_TRACE() << "Session[" << name_ << "] at " << this
                    << " fd: " << socket() << " free.";
    }

    auto Session::shutdown() -> Error
    {
        if (state_ == STATE_CONNECTED) {
            setState(STATE_DISCONNECTING);
            if (!event_->checkFlag(EVENT_WRITE)) {
                return socket_->shutdownWrite();
            }
            return Error(HARE_ERROR_SOCKET_WRITING);
        }
        return Error(HARE_ERROR_SESSION_ALREADY_DISCONNECT);
    }

    auto Session::forceClose() -> Error
    {
        if (state_ == STATE_CONNECTED || state_ == STATE_DISCONNECTING) {
            setState(STATE_DISCONNECTING);
            handleClose();
        }
        return Error(HARE_ERROR_SESSION_ALREADY_DISCONNECT);
    }

    void Session::startRead()
    {
        if (!reading_ || !event_->checkFlag(EVENT_READ)) {
            event_->setFlags(EVENT_READ);
            reading_ = true;
        }
    }

    void Session::stopRead()
    {
        if (reading_ || event_->checkFlag(EVENT_READ)) {
            event_->clearFlags(EVENT_READ);
            reading_ = false;
        }
    }

    Session::Session(core::Cycle* cycle, Socket::TYPE type,
        HostAddress local_addr,
        std::string name, int8_t family, util_socket_t target_fd,
        HostAddress peer_addr)
        : cycle_(HARE_CHECK_NULL(cycle))
        , socket_(new Socket(family, type, target_fd))
        , local_addr_(std::move(local_addr))
        , peer_addr_(std::move(peer_addr))
        , name_(std::move(name))
    {
    }

    void Session::handleClose()
    {
        LOG_TRACE() << "(handleClose) fd = " << socket() << " state = " << detail::stateToString(state_);
        HARE_ASSERT(state_ == STATE_CONNECTED || state_ == STATE_DISCONNECTING, "");
        // we don't close fd, leave it to dtor, so we can find leaks easily.
        setState(STATE_DISCONNECTED);
        event_->clearAllFlags();

        connection(EVENT_CLOSED);
    }

    void Session::handleError()
    {
        SYS_ERROR() << "Occurred error to the session[" << name_ << "], detail: " << socket::getSocketErrorInfo(socket());
        connection(EVENT_ERROR);
    }

    void Session::connectEstablished()
    {
        HARE_ASSERT(state_ == STATE_CONNECTING, "");
        setState(STATE_CONNECTED);
        event_->setFlags(EVENT_READ);
        connection(EVENT_CONNECTED);
    }

    void Session::connectDestroyed()
    {
        if (state_ == STATE_CONNECTED) {
            setState(STATE_DISCONNECTED);
            event_->clearAllFlags();
            connection(EVENT_CLOSED);
        }
        event_->deactive();
    }

} // namespace net
} // namespace hare