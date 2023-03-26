#include "hare/net/tcp_session_p.h"
#include "hare/net/tcp_session.h"
#include "hare/net/socket_op.h"

#include <hare/base/logging.h>

namespace hare {
namespace net {

    namespace detail {

        TcpEvent::TcpEvent(core::Cycle* cycle, util_socket_t target_fd, TcpSessionPrivate* tsp)
            : Event(cycle, target_fd)
            , tsp_(tsp)
        {
        }

        TcpEvent::~TcpEvent() = default;

        void TcpEvent::eventCallBack(int32_t events, const Timestamp& receive_time)
        {
            ownerCycle()->assertInCycleThread();
            LOG_TRACE() << "tcp session[" << fd() << "] revents: " << rflagsToString();
            if (auto tied_object = tiedObject().lock()) {
                auto* tied_session = static_cast<TcpSession*>(tied_object.get());
                if ((events & EVENT_READ) != 0) {
                    tsp_->handleRead(tied_session, receive_time);
                }
                if ((events & EVENT_WRITE) != 0) {
                    tsp_->handleWrite(tied_session);
                }
                if ((events & EVENT_CLOSED) != 0) {
                    tsp_->handleClose(tied_session);
                }
                if ((events & EVENT_ERROR) != 0) {
                    tsp_->handleError(tied_session);
                }
            } else {
                SYS_ERROR() << "The tied tcp session was not found.";
            }
        }

    } // namespace detail

    void TcpSessionPrivate::handleRead(TcpSession* session, const Timestamp& receive_time)
    {
        auto read_n = in_buffer_.read(event_->fd(), -1);
        if (read_n == 0) {
            HARE_ASSERT(state_ != SE_STATE::CONNECTING, "");
            state_ = SE_STATE::CONNECTING;
            event_->clearAllFlags();
            session->connection(EVENT_CLOSED);
        } else if (read_n > 0) {
            session->read(in_buffer_, receive_time);
        } else {
            handleError(session);
        }
    }

    void TcpSessionPrivate::handleWrite(TcpSession* session)
    {
        if (event_->checkFlag(EVENT_WRITE)) {
            std::unique_lock<std::mutex> locker(out_buffer_mutex_);
            auto write_n = out_buffer_.write(event_->fd(), -1);
            if (write_n > 0) {
                if (out_buffer_.length() == 0) {
                    locker.unlock();
                    event_->clearFlags(EVENT_WRITE);
                    cycle_->queueInLoop(std::bind(&TcpSession::writeComplete, session->shared_from_this()));
                } else {
                    locker.unlock();
                }
                if (state_ == SE_STATE::DISCONNECTING) {
                    session->shutdownInCycle();
                }
            } else {
                locker.unlock();
                SYS_ERROR() << "An error occurred while writing the socket, detail: " << socket::getSocketErrorInfo(event_->fd());
            }
        } else {
            LOG_TRACE() << "Connection[fd: " << event_->fd() << "] is down, no more writing";
        }
    }

    void TcpSessionPrivate::handleClose(TcpSession* session)
    {
        HARE_ASSERT(state_ != SE_STATE::CONNECTING, "");
        state_ = SE_STATE::CONNECTING;
        event_->clearAllFlags();
        session->connection(EVENT_CLOSED);
    }

    void TcpSessionPrivate::handleError(TcpSession* session)
    {
        SYS_ERROR() << "An error occurred while reading the socket, detail: " << socket::getSocketErrorInfo(event_->fd());
        session->connection(EVENT_ERROR);
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
            std::unique_lock<std::mutex> locker(p_->out_buffer_mutex_);
            auto empty = p_->out_buffer_.length() == 0;
            p_->out_buffer_.add(bytes, length);
            if (empty) {
                locker.unlock();
                if (p_->cycle_->isInCycleThread()) {
                    p_->cycle_->queueInLoop(std::bind(&TcpSession::writeInCycle, shared_from_this()));
                } else {
                    p_->cycle_->runInLoop(std::bind(&TcpSession::writeInCycle, shared_from_this()));
                }
            } else if (p_->out_buffer_.length() > p_->high_water_mark_) {
                locker.unlock();
                if (p_->cycle_->isInCycleThread()) {
                    p_->cycle_->queueInLoop(std::bind(&TcpSession::highWaterMark, shared_from_this()));
                } else {
                    p_->cycle_->runInLoop(std::bind(&TcpSession::highWaterMark, shared_from_this()));
                }
            }
        }
        return false;
    }

    void TcpSession::shutdown()
    {
        // FIXME: use atomic compare and swap
        if (p_->state_ != SE_STATE::CONNECTED) {
            p_->state_ = SE_STATE::DISCONNECTING;
            p_->cycle_->runInLoop(std::bind(&TcpSession::shutdownInCycle, this));
        }
    }

    void TcpSession::forceClose()
    {
        if (p_->state_ != SE_STATE::CONNECTING) {
            p_->state_ = SE_STATE::DISCONNECTING;
            p_->cycle_->queueInLoop(std::bind(&TcpSession::forceCloseInCycle, shared_from_this()));
        }
    }

    void TcpSession::setTcpNoDelay(bool tcp_on)
    {
        p_->socket_->setTcpNoDelay(tcp_on);
    }

    void TcpSession::startRead()
    {
        p_->cycle_->runInLoop(std::bind(&TcpSession::startReadInCycle, shared_from_this()));
    }

    void TcpSession::stopRead()
    {
        p_->cycle_->runInLoop(std::bind(&TcpSession::stopReadInCycle, shared_from_this()));
    }

    TcpSession::TcpSession(TcpSessionPrivate* tsp)
        : p_(tsp)
    {
        HARE_ASSERT(p_ != nullptr, "The private of TcpSession is NULL!");
        LOG_DEBUG() << "TcpSession::ctor[" << p_->name_ << "] at " << this
                    << " fd: " << p_->socket_->socket();
    }

    core::Cycle* TcpSession::getCycle()
    {
        return p_->cycle_;
    }

    void TcpSession::shutdownInCycle()
    {
        p_->cycle_->assertInCycleThread();
        if (!p_->event_->checkFlag(EVENT_WRITE)) {
            // we are not writing
            p_->socket_->shutdownWrite();
        }
    }

    void TcpSession::forceCloseInCycle()
    {
        p_->cycle_->assertInCycleThread();
        if (state() != SE_STATE::CONNECTING) {
            // as if we received 0 byte in handleRead();
            p_->handleClose(this);
        }
    }

    void TcpSession::startReadInCycle()
    {
        p_->cycle_->assertInCycleThread();
        if (!p_->reading_ || !p_->event_->checkFlag(EVENT_READ)) {
            p_->event_->setFlags(EVENT_READ);
            p_->reading_ = true;
        }
    }

    void TcpSession::stopReadInCycle()
    {
        p_->cycle_->assertInCycleThread();
        if (p_->reading_ || p_->event_->checkFlag(EVENT_READ)) {
            p_->event_->clearFlags(EVENT_READ);
            p_->reading_ = false;
        }
    }

    void TcpSession::writeInCycle()
    {
        p_->handleWrite(this);
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
