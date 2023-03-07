#include "hare/net/tcp_session_p.h"
#include "hare/net/tcp_session.h"
#include "hare/net/socket_op.h"

#include <hare/base/logging.h>

namespace hare {
namespace net {

    namespace detail {

        TcpEvent::TcpEvent(core::Cycle* cycle, util_socket_t fd)
            : Event(cycle, fd)
        {
        }

        TcpEvent::~TcpEvent() = default;

        void TcpEvent::eventCallBack(int32_t events, const Timestamp& receive_time)
        {
            ownerCycle()->assertInCycleThread();
            LOG_TRACE() << "tcp session[" << fd() << "] revents: " << rflagsToString();
            if (auto tied_object = tiedObject().lock()) {
                auto tied_session = static_cast<TcpSession*>(tied_object.get());
                if (events & (EVENT_CLOSED | EVENT_ERROR))
                    tied_session->connection(events);
                if (events & EVENT_READ) {
                    auto n = tied_session->in_buffer().read(fd(), -1);
                    if (n == 0) {
                        tied_session->connection(events | EVENT_CLOSED);
                    } else if (n > 0) {
                        tied_session->read(tied_session->in_buffer(), receive_time);
                    } else {
                        SYS_ERROR() << "An error occurred while reading the socket, detail: " << socket::getSocketErrorInfo(fd());
                        tied_session->connection(events | EVENT_ERROR);
                    }
                }
                if (events & EVENT_WRITE) {
                    if (flags() & EVENT_WRITE) {
                        auto& out_buffer = tied_session->out_buffer();
                        auto n = out_buffer.write(fd(), -1);
                        if (n > 0) {
                            if (out_buffer.length() == 0) {
                                clearFlags(EVENT_WRITE);
                                ownerCycle()->queueInLoop(std::bind(&TcpSession::writeComplete, tied_session->shared_from_this()));
                            }
                        } else {
                            SYS_ERROR() << "An error occurred while writing the socket, detail: " << socket::getSocketErrorInfo(fd());
                        }
                    } else {
                        LOG_TRACE() << "Connection fd: " << fd()
                                    << " is down, no more writing";
                    }
                }
            } else {
                SYS_ERROR() << "The tied tcp session was not found.";
            }
        }

    } // namespace detail

    TcpSession::~TcpSession()
    {
        LOG_DEBUG() << "TcpSession[" << p_->name_ << "] at " << this
                    << " fd: " << p_->socket_->socket();
        delete p_;
    }

    const std::string& TcpSession::name() const
    {
        return p_->name_;
    }

    const HostAddress& TcpSession::local_address() const
    {
        return p_->local_addr_;
    }

    const HostAddress& TcpSession::peer_address() const
    {
        return p_->peer_addr_;
    }

    void TcpSession::setHighWaterMark(std::size_t high_water)
    {
        p_->high_water_mark_ = high_water;
    }

    Buffer& TcpSession::in_buffer()
    {
        return p_->input_buffer_;
    }

    Buffer& TcpSession::out_buffer()
    {
        return p_->output_buffer_;
    }

    TcpSession::TcpSession(TcpSessionPrivate* p)
        : p_(p)
    {
        HARE_ASSERT(p_ != nullptr, "The private of TcpSession is NULL!");

        LOG_DEBUG() << "TcpSession::ctor[" <<  p_->name_ << "] at " << this
                    << " fd=" << p_->socket_->socket();

        p_->socket_->setKeepAlive(true);
        p_->event_.reset(new detail::TcpEvent(p_->cycle_, p_->socket_->socket()));
        p_->event_->setFlags(EVENT_READ);
    }

    void TcpSession::connectEstablished()
    {
        p_->cycle_->assertInCycleThread();

        p_->event_->tie(shared_from_this());
        p_->event_->setFlags(EVENT_READ);
        connection(EVENT_CONNECTED);
    }

} // namespace net
} // namespace hare
