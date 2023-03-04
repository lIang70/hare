#ifndef _HARE_NET_TCP_SESSION_P_H_
#define _HARE_NET_TCP_SESSION_P_H_

#include "hare/net/core/cycle.h"
#include "hare/net/core/event.h"
#include <hare/net/host_address.h>
#include <hare/net/socket.h>
#include <hare/net/tcp_session.h>

#include <atomic>

namespace hare {
namespace net {

    namespace detail {

        class TcpEvent : public core::Event {
            std::weak_ptr<TcpSession> session_ {};

        public:
            TcpEvent(core::Cycle* cycle, util_socket_t fd, STcpSession session);
            ~TcpEvent() override;

            void eventCallBack(int32_t events, Timestamp& receive_time) override;
        };

    } // namespace detail

    class TcpSessionPrivate {
    public:
        core::Cycle* cycle_ { nullptr };
        const std::string name_ {};
        std::atomic<bool> reading_ { false };

        std::unique_ptr<Socket> socket_ { nullptr };
        std::shared_ptr<core::Event> event_ { nullptr };
        const HostAddress local_addr_ {};
        const HostAddress peer_addr_ {};

        std::size_t high_water_mark_ { 64 * 1024 * 1024 };
        Buffer input_buffer_ {};
        Buffer output_buffer_ {};

        TcpSessionPrivate(core::Cycle* cycle,
            const std::string& name, util_socket_t fd,
            const HostAddress& local_addr,
            const HostAddress& peer_addr)
            : cycle_(cycle)
            , name_(name)
            , socket_(new Socket(fd))
            , local_addr_(local_addr)
            , peer_addr_(peer_addr)
        {
        }
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SESSION_P_H_