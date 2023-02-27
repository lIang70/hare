#ifndef _HARE_NET_TCP_SERVE_P_H_
#define _HARE_NET_TCP_SERVE_P_H_

#include "hare/net/core/event.h"
#include "hare/net/core/cycle_threadpool.h"
#include <hare/net/tcp_serve.h>

namespace hare {
namespace net {

    namespace detail {

        class Acceptor : core::Event {

        public:
            Acceptor(core::Cycle* cycle, util_socket_t fd)
                : Event(cycle, fd)
            {
            }

        protected:
            void eventCallBack(int32_t events, Timestamp& receive_time) override;

        };
        
    } // namespace detail
    
    class TcpServePrivate {
        std::unique_ptr<detail::Acceptor> acceptor_ { nullptr };
        std::shared_ptr<core::CycleThreadPool> thread_pool_ { nullptr };
        // the acceptor loop
        core::Cycle* cycle_ {};
        const std::string ip_port_ {};
        const std::string name_ {};
        uint64_t session_id_ { 0 };
        std::atomic<bool> started_ { false };


    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SERVE_P_H_
