#ifndef _HARE_NET_TCP_SESSION_P_H_
#define _HARE_NET_TCP_SESSION_P_H_

#include "hare/net/core/cycle.h"
#include "hare/net/core/event.h"
#include <hare/net/host_address.h>
#include <hare/net/socket.h>
#include <hare/net/tcp_session.h>

#include <atomic>
#include <mutex>

namespace hare {
namespace net {

    using SE_STATE = TcpSession::State;

    namespace detail {

        class TcpEvent : public core::Event {
            TcpSessionPrivate* tsp_ { nullptr };

        public:
            TcpEvent(core::Cycle* cycle, util_socket_t target_fd, TcpSessionPrivate* tsp);
            ~TcpEvent() override;

            void eventCallBack(int32_t events, const Timestamp& receive_time) override;
        };

    } // namespace detail

    class TcpSessionPrivate {
    public:
        core::Cycle* cycle_ { nullptr };
        const std::string name_ {};
        std::atomic<bool> reading_ { false };
        SE_STATE state_ { SE_STATE::CONNECTING };

        std::unique_ptr<Socket> socket_ { nullptr };
        std::shared_ptr<detail::TcpEvent> event_ { nullptr };
        const HostAddress local_addr_ {};
        const HostAddress peer_addr_ {};


        Buffer in_buffer_ {};
        std::mutex out_buffer_mutex_ {};
        Buffer out_buffer_ {};
        std::size_t high_water_mark_ { TcpSession::DEFAULT_HIGH_WATER };

        TcpSessionPrivate(core::Cycle* cycle,
            std::string name, int8_t family, util_socket_t target_fd,
            HostAddress local_addr,
            HostAddress peer_addr)
            : cycle_(cycle)
            , name_(std::move(name))
            , socket_(new Socket(family, target_fd))
            , local_addr_(std::move(local_addr))
            , peer_addr_(std::move(peer_addr))
        {
            socket_->setKeepAlive(true);
            event_.reset(new detail::TcpEvent(cycle_, socket_->socket(), this));
            event_->setFlags(EVENT_READ);
        }

        void handleRead(TcpSession* session, const Timestamp& receive_time);
        void handleWrite(TcpSession* session);
        void handleClose(TcpSession* session);
        void handleError(TcpSession* session);

    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SESSION_P_H_