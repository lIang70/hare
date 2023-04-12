#ifndef _HARE_NET_HYBRID_SERVE_H_
#define _HARE_NET_HYBRID_SERVE_H_

#include <hare/net/acceptor.h>
#include <hare/net/core/timer.h>
#include <hare/net/session.h>

#include <map>
#include <memory>

namespace hare {
namespace net {

    class HARE_API HybridServe  : public NonCopyable
                                , public std::enable_shared_from_this<HybridServe> {
        std::string name_ {};
        std::string reactor_type_ {};

        // the acceptor loop
        std::shared_ptr<core::Cycle> cycle_ { nullptr };

        uint64_t session_id_ { 0 };

        std::map<util_socket_t, Acceptor::Ptr> acceptors_ {};

        bool started_ { false };

    public:
        using Ptr = std::shared_ptr<HybridServe>;

        //! @brief Construct a new Tcp Serve object
        //! @param type The type of reactor. EPOLL/POLL
        explicit HybridServe(const std::string& type, const std::string& name = "HARE_SERVE");
        virtual ~HybridServe();

        auto isRunning() const -> bool { return started_; }

        auto addAcceptor(const Acceptor::Ptr& acceptor) -> bool;
        void removeAcceptor(util_socket_t acceptor_socket);

        auto add(core::Timer* timer) -> core::Timer::Id;
        void cancel(core::Timer::Id timer_id);

        void exec();
        void exit();

    protected:
        virtual void newSessionConnected(Session::Ptr session, Timestamp time, const Acceptor::Ptr& acceptor) = 0;

    private:
        void activeAcceptors();
        void newSession(util_socket_t target_fd, HostAddress& address, const Timestamp& time, util_socket_t acceptor_socket);
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SERVE_H_