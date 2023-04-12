#include <functional>
#include <hare/net/tcp_session.h>

#include "hare/base/error.h"
#include "hare/net/core/cycle.h"
#include "hare/net/session.h"
#include "hare/net/socket.h"
#include "hare/net/socket_op.h"
#include <hare/base/logging.h>
#include <memory>

namespace hare {
namespace net {

    namespace detail {

        class TcpEvent : public core::Event {
            std::function<void(const Timestamp& receive_time)> handle_read_ {};
            core::Timer::Task handle_write_ {};
            core::Timer::Task handle_close_ {};
            core::Timer::Task handle_error_ {};

        public:
            TcpEvent(core::Cycle* cycle, util_socket_t target_fd)
                : Event(cycle, target_fd)
            {
            }
            ~TcpEvent() override = default;

            void eventCallBack(int32_t events, const Timestamp& receive_time) override;

            friend class net::TcpSession;
        };

        void TcpEvent::eventCallBack(int32_t events, const Timestamp& receive_time)
        {
            LOG_TRACE() << "tcp session[" << fd() << "] revents: " << rflagsToString();
            auto* tied_session = static_cast<TcpSession*>(tiedObject().lock().get());
            if (tied_session == nullptr) {
                SYS_ERROR() << "The tied tcp session was not found.";
            }
            if ((events & EVENT_READ) != 0) {
                handle_read_(receive_time);
            }
            if ((events & EVENT_WRITE) != 0) {
                handle_write_();
            }
            if ((events & EVENT_CLOSED) != 0) {
                handle_close_();
            }
            if ((events & EVENT_ERROR) != 0) {
                handle_error_();
            }
        }

    } // namespace detail

    TcpSession::~TcpSession() = default;

    auto TcpSession::send(const uint8_t* bytes, std::size_t length) -> bool
    {
        if (state() == STATE_CONNECTED) {
            auto empty = out_buffer_.length() == 0;
            out_buffer_.add(bytes, length);
            if (empty) {
                cycle_->queueInLoop(std::bind([](const std::weak_ptr<TcpSession>& session) {
                    auto tcp = session.lock();
                    if (tcp) {
                        tcp->handleWrite();
                    }
                },
                    shared_from_this()));
            } else if (out_buffer_.length() > high_water_mark_) {
                cycle_->queueInLoop(std::bind([](const std::weak_ptr<TcpSession>& session) {
                    auto tcp = session.lock();
                    if (tcp) {
                        tcp->highWaterMark();
                    }
                },
                    shared_from_this()));
            }
        }
        return false;
    }

    auto TcpSession::setTcpNoDelay(bool tcp_on) -> Error
    {
        return socket_->setTcpNoDelay(tcp_on);
    }

    TcpSession::TcpSession(core::Cycle* cycle,
        HostAddress local_addr,
        std::string name, int8_t family, util_socket_t target_fd,
        HostAddress peer_addr)
        : Session(cycle, Socket::TYPE_TCP,
            std::move(local_addr),
            std::move(name), family, target_fd,
            std::move(peer_addr))
    {
        event_.reset(new detail::TcpEvent(cycle, target_fd));
        event_->tie(shared_from_this());

        auto* tcp_event = dynamic_cast<detail::TcpEvent*>(event_.get());
        tcp_event->handle_read_ = (std::bind(&TcpSession::handleRead, this, std::placeholders::_1));
        tcp_event->handle_write_ = (std::bind(&TcpSession::handleWrite, this));
        tcp_event->handle_close_ = (std::bind(&TcpSession::handleClose, this));
        tcp_event->handle_error_ = (std::bind(&TcpSession::handleError, this));

        socket_->setKeepAlive(true);
    }

    void TcpSession::handleRead(const Timestamp& receive_time)
    {
        auto read_n = in_buffer_.read(event_->fd(), -1);
        if (read_n == 0) {
            handleClose();
        } else if (read_n > 0) {
            read(in_buffer_, receive_time);
        } else {
            handleError();
        }
    }

    void TcpSession::handleWrite()
    {
        if (event_->checkFlag(EVENT_WRITE)) {
            auto write_n = out_buffer_.write(event_->fd(), -1);
            if (write_n > 0) {
                if (out_buffer_.length() == 0) {
                    event_->clearFlags(EVENT_WRITE);
                    cycle_->queueInLoop(std::bind([](const std::weak_ptr<TcpSession>& session) {
                        auto tcp = session.lock();
                        if (tcp) {
                            tcp->writeComplete();
                        }
                    },
                        shared_from_this()));
                }
                if (state_ == STATE_DISCONNECTING) {
                    shutdown();
                }
            } else {
                SYS_ERROR() << "An error occurred while writing the socket, detail: " << socket::getSocketErrorInfo(event_->fd());
            }
        } else {
            LOG_TRACE() << "Connection[fd: " << event_->fd() << "] is down, no more writing";
        }
    }

} // namespace net
} // namespace hare
