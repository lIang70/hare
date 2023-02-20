#include <hare/base/logging.h>
#include <hare/net/tcp_session_p.h>

namespace hare {
namespace net {

    namespace detail {

        TcpEvent::TcpEvent(core::Cycle* cycle, util_socket_t fd, TcpSession* session)
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

    TcpSession::TcpSession(TcpSessionPrivate* p)
        : p_(p)
    {
        if (!p)
            LOG_FATAL() << "TcpSessionPrivate is NULL!";

        p_->event_.reset(new detail::TcpEvent(p_->cycle_, p_->socket_->socket(), this));
    }

} // namespace net
} // namespace hare
