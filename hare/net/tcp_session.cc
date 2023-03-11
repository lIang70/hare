#include "hare/net/tcp_session_p.h"
#include "hare/net/tcp_session.h"
#include "hare/net/socket_op.h"

#include <hare/base/logging.h>

namespace hare {
namespace net {

    namespace detail {

        TcpEvent::TcpEvent(core::Cycle* cycle, util_socket_t fd, TcpSessionPrivate* tsp)
            : Event(cycle, fd)
            , tsp_(tsp)
        {
        }

        TcpEvent::~TcpEvent() = default;

        void TcpEvent::eventCallBack(int32_t events, const Timestamp& receive_time)
        {
            ownerCycle()->assertInCycleThread();
            LOG_TRACE() << "tcp session[" << fd() << "] revents: " << rflagsToString();
            if (auto tied_object = tiedObject().lock()) {
                auto tied_session = static_cast<TcpSession*>(tied_object.get());
                if (events & EVENT_READ) {
                    tsp_->handleRead(tied_session, receive_time);
                }
                if (events & EVENT_WRITE) {
                    tsp_->handleWrite(tied_session);
                }
                if (events & EVENT_CLOSED) {
                    tsp_->handleClose(tied_session);
                }
                if (events & EVENT_ERROR) {
                    tsp_->handleError(tied_session);
                }
            } else {
                SYS_ERROR() << "The tied tcp session was not found.";
            }
        }

    } // namespace detail

    void TcpSessionPrivate::handleRead(TcpSession* ts, const Timestamp& receive_time)
    {
        auto n = event_->inBuffer().read(event_->fd(), -1);
        if (n == 0) {
            HARE_ASSERT(state_ != SE_STATE::CONNECTING, "");
            state_ = SE_STATE::CONNECTING;
            event_->clearAllFlags();
            ts->connection(EVENT_CLOSED);
        } else if (n > 0) {
            ts->read(event_->inBuffer(), receive_time);
        } else {
            handleError(ts);
        }
    }

    void TcpSessionPrivate::handleWrite(TcpSession* ts)
    {
        if (event_->checkFlag(EVENT_WRITE)) {
            auto n = event_->outBuffer().write(event_->fd(), -1);
            if (n > 0) {
                if (event_->outBuffer().length() == 0) {
                    event_->clearFlags(EVENT_WRITE);
                    cycle_->queueInLoop(std::bind(&TcpSession::writeComplete, ts->shared_from_this()));
                }
                if (state_ == SE_STATE::DISCONNECTING) {
                    ts->shutdownInLoop();
                }
            } else {
                SYS_ERROR() << "An error occurred while writing the socket, detail: " << socket::getSocketErrorInfo(event_->fd());
            }
        } else {
            LOG_TRACE() << "Connection[fd: " << event_->fd() << "] is down, no more writing";
        }
    }

    void TcpSessionPrivate::handleClose(TcpSession* ts)
    {
        HARE_ASSERT(state_ != SE_STATE::CONNECTING, "");
        state_ = SE_STATE::CONNECTING;
        event_->clearAllFlags();
        ts->connection(EVENT_CLOSED);
    }

    void TcpSessionPrivate::handleError(TcpSession* ts)
    {
        SYS_ERROR() << "An error occurred while reading the socket, detail: " << socket::getSocketErrorInfo(event_->fd());
        ts->connection(EVENT_ERROR);
    }

    TcpSession::~TcpSession()
    {
        HARE_ASSERT(p_->state_ == SE_STATE::CONNECTING, "");
        LOG_DEBUG() << "TcpSession[" << p_->name_ << "] at " << this
                    << " fd: " << p_->socket_->socket();
        delete p_;
    }

    const std::string& TcpSession::name() const
    {
        return p_->name_;
    }

    const HostAddress& TcpSession::localAddress() const
    {
        return p_->local_addr_;
    }

    const HostAddress& TcpSession::peerAddress() const
    {
        return p_->peer_addr_;
    }

    void TcpSession::setHighWaterMark(std::size_t high_water)
    {
        p_->high_water_mark_ = high_water;
    }

    SE_STATE TcpSession::state() const
    {
        return p_->state_;
    }

    util_socket_t TcpSession::socket() const
    {
        return p_->socket_->socket();
    }

    bool TcpSession::send(const uint8_t* bytes, std::size_t length)
    {
        if (p_->state_ == SE_STATE::CONNECTED) {

        }
        return false;
    }

    void TcpSession::shutdown()
    {
        // FIXME: use atomic compare and swap
        if (p_->state_ != SE_STATE::CONNECTED) {
            p_->state_ = SE_STATE::DISCONNECTING;
            p_->cycle_->runInLoop(std::bind(&TcpSession::shutdownInLoop, this));
        }
    }

    void TcpSession::forceClose()
    {
        if (p_->state_ != SE_STATE::CONNECTING) {
            p_->state_ = SE_STATE::DISCONNECTING;
            p_->cycle_->queueInLoop(std::bind(&TcpSession::forceCloseInLoop, shared_from_this()));
        }
    }

    void TcpSession::setTcpNoDelay(bool on)
    {
        p_->socket_->setTcpNoDelay(on);
    }

    void TcpSession::startRead()
    {
        p_->cycle_->runInLoop(std::bind(&TcpSession::startReadInLoop, shared_from_this()));
    }

    void TcpSession::stopRead()
    {
        p_->cycle_->runInLoop(std::bind(&TcpSession::stopReadInLoop, shared_from_this()));
    }

    TcpSession::TcpSession(TcpSessionPrivate* p)
        : p_(p)
    {
        HARE_ASSERT(p_ != nullptr, "The private of TcpSession is NULL!");
        LOG_DEBUG() << "TcpSession::ctor[" << p_->name_ << "] at " << this
                    << " fd: " << p_->socket_->socket();
    }

    void TcpSession::shutdownInLoop()
    {
        p_->cycle_->assertInCycleThread();
        if (!p_->event_->checkFlag(EVENT_WRITE)) {
            // we are not writing
            p_->socket_->shutdownWrite();
        }
    }

    void TcpSession::forceCloseInLoop()
    {
        p_->cycle_->assertInCycleThread();
        if (state() != SE_STATE::CONNECTING) {
            // as if we received 0 byte in handleRead();
            p_->handleClose(this);
        }
    }

    void TcpSession::startReadInLoop()
    {
        p_->cycle_->assertInCycleThread();
        if (!p_->reading_ || !p_->event_->checkFlag(EVENT_READ)) {
            p_->event_->setFlags(EVENT_READ);
            p_->reading_ = true;
        }
    }

    void TcpSession::stopReadInLoop()
    {
        p_->cycle_->assertInCycleThread();
        if (p_->reading_ || p_->event_->checkFlag(EVENT_READ)) {
            p_->event_->clearFlags(EVENT_READ);
            p_->reading_ = false;
        }
    }

    void TcpSession::connectEstablished()
    {
        p_->cycle_->assertInCycleThread();
        HARE_ASSERT(state() == SE_STATE::CONNECTING, "");
        p_->state_ = SE_STATE::CONNECTED;
        p_->event_->tie(shared_from_this());
        p_->event_->setFlags(EVENT_READ);
        connection(EVENT_CONNECTED);
    }

    void TcpSession::connectDestroyed()
    {
        p_->cycle_->assertInCycleThread();
        if (state() == SE_STATE::CONNECTED) {
            p_->state_ = SE_STATE::CONNECTING;
            p_->event_->clearAllFlags();
            connection(EVENT_CLOSED);
        }
        p_->event_->deactive();
    }

} // namespace net
} // namespace hare
