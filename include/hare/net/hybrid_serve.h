#ifndef _HARE_NET_HYBRID_SERVE_H_
#define _HARE_NET_HYBRID_SERVE_H_

#include <hare/net/acceptor.h>
#include <hare/net/session.h>

#include <map>

namespace hare {
namespace net {

    class HARE_API hybrid_serve : public non_copyable
                                , public std::enable_shared_from_this<hybrid_serve> {
        std::string name_ {};

        // the acceptor loop
        ptr<io::cycle> cycle_ {};
        uint64_t session_id_ { 0 };
        std::map<util_socket_t, acceptor::ptr> acceptors_ {};
        bool started_ { false };

    public:
        using ptr = ptr<hybrid_serve>;

        explicit hybrid_serve(hare::ptr<io::cycle> _cycle, std::string _name = "HARE_SERVE");
        virtual ~hybrid_serve();

        auto is_running() const -> bool { return started_; }

        auto add_acceptor(const acceptor::ptr& _acceptor) -> bool;
        void remove_acceptor(util_socket_t _fd);

        void exec();
        void exit();

    protected:
        virtual void new_session_connected(session::ptr _session, timestamp _time, const acceptor::ptr& _acceptor) = 0;

    private:
        void active_acceptors();
        void new_session(util_socket_t _fd, host_address& _address, const timestamp& _time, util_socket_t _acceptor);
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SERVE_H_