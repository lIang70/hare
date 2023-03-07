#ifndef _HARE_NET_TCP_SERVE_P_H_
#define _HARE_NET_TCP_SERVE_P_H_

#include "hare/net/core/event.h"
#include "hare/net/core/cycle_threadpool.h"
#include <hare/net/socket.h>
#include <hare/net/tcp_serve.h>

#include <hare/base/logging.h>

#ifdef HARE__HAVE_FCNTL_H
#include <fcntl.h>
#endif

namespace hare {
namespace net {

    namespace detail {

        class Acceptor : public core::Event {
            using NewSession = std::function<void(util_socket_t, const HostAddress& conn_address, const Timestamp&)>;
        public:
            Socket socket_ { -1 };
            NewSession new_session_ {};
#ifdef H_OS_LINUX
            // Read the section named "The special problem of
            // accept()ing when you can't" in libev's doc.
            // By Marc Lehmann, author of libev.
            util_socket_t idle_fd_ { -1 };
#endif

            Acceptor(core::Cycle* cycle, util_socket_t fd, bool reuse_port)
                : Event(cycle, fd)
                , socket_(fd)
#ifdef H_OS_LINUX
                , idle_fd_( ::open("/dev/null", O_RDONLY | O_CLOEXEC) )
            {
                HARE_ASSERT(idle_fd_ > 0, "");
#else
            {
#endif
                socket_.setReuseAddr(true);
                socket_.setReusePort(reuse_port);
            }

            ~Acceptor() override
            {
                clearAllFlags();
                deactive();
#ifdef H_OS_LINUX
                socket::close(idle_fd_);
#endif
            }

            inline void listen()
            {
                socket_.listen();
                setFlags(EVENT_READ);
            }

        protected:
            void eventCallBack(int32_t events, const Timestamp& receive_time) override;

        };
        
    } // namespace detail
    
    class TcpServePrivate {
    public:
        std::string name_ {};
        // the acceptor loop
        std::unique_ptr<core::Cycle> cycle_ { nullptr };
        std::unique_ptr<detail::Acceptor> acceptor_ { nullptr };
        std::shared_ptr<core::CycleThreadPool> thread_pool_ { nullptr };

        std::string reactor_type_ {};
        HostAddress listen_address_ {};
        bool reuse_port_ { false };
        int8_t family_ { 0 };
        int32_t thread_num_ { 0 };
        uint64_t session_id_ { 0 };

        std::atomic<bool> started_ { false };

    public:
        void newSession(util_socket_t fd, const HostAddress& address, const Timestamp& ts);

    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SERVE_P_H_
