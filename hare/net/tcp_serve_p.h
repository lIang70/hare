#ifndef _HARE_NET_TCP_SERVE_P_H_
#define _HARE_NET_TCP_SERVE_P_H_

#include <hare/net/acceptor.h>
#include "hare/net/core/cycle_threadpool.h"
#include "hare/net/core/event.h"
#include "hare/net/socket_op.h"
#include <hare/base/logging.h>
#include <hare/net/socket.h>
#include <hare/net/tcp_serve.h>

#include <cstdint>
#include <map>

namespace hare {
namespace net {

    class TcpServePrivate {
        friend class TcpServe;

        TcpServe* serve_ { nullptr }; 
        std::string name_ {};
        // the acceptor loop
        std::unique_ptr<core::Cycle> cycle_ { nullptr };
        std::shared_ptr<core::CycleThreadPool> thread_pool_ { nullptr };

        std::string reactor_type_ {};
        int32_t thread_num_ { 0 };
        uint64_t session_id_ { 0 };

        std::map<util_socket_t, Acceptor::Ptr> acceptors_ {};

        std::atomic<bool> started_ { false };

        void newSession(util_socket_t target_fd, int8_t family, const HostAddress& address, const Timestamp& time);
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SERVE_P_H_
