#include "hare/net/tcp_session_p.h"
#include <hare/base/logging.h>

namespace hare {
namespace net {

    namespace detail {

        TcpEvent::TcpEvent(core::Cycle* cycle, util_socket_t fd, STcpSession session)
            : Event(cycle, fd)
            , session_(session)
        {
        }

        TcpEvent::~TcpEvent()
        {
        }

        void TcpEvent::eventCallBack(int32_t events, Timestamp& receive_time)
        {
        }

    } // namespace detail

    TcpSession::~TcpSession()
    {
        LOG_DEBUG() << "TcpSession[" << p_->name_ << "] at " << this
                    << " fd: " << p_->socket_->socket();
        delete p_;
    }

    void TcpSession::setHighWaterMark(std::size_t high_water)
    {
        p_->high_water_mark_ = high_water;
    }

    TcpSession::TcpSession(TcpSessionPrivate* p)
        : p_(p)
    {
        HARE_ASSERT(p_ != nullptr, "The private of TcpSession is NULL!");

        LOG_DEBUG() << "TcpSession::ctor[" <<  p_->name_ << "] at " << this
                    << " fd=" << p_->socket_->socket();

        p_->socket_->setKeepAlive(true);
        p_->event_.reset(new detail::TcpEvent(p_->cycle_, p_->socket_->socket(), shared_from_this()));
    }

} // namespace net
} // namespace hare
