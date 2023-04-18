#ifndef _HARE_NET_HYBRID_SERVE_H_
#define _HARE_NET_HYBRID_SERVE_H_

#include <hare/net/acceptor.h>
#include <hare/net/session.h>

#include <map>

namespace hare {
namespace net {

    class io_pool;
    class HARE_API hybrid_serve : public non_copyable
                                , public std::enable_shared_from_this<hybrid_serve> {
        using new_session_callback = std::function<void(session::ptr, timestamp, const acceptor::ptr&)>;
        
        std::string name_ {};

        // the acceptor loop
        ptr<io::cycle> cycle_ {};
        ptr<io_pool> io_pool_ {};
        uint64_t session_id_ { 0 };
        std::map<util_socket_t, acceptor::ptr> acceptors_ {};
        bool started_ { false };

        new_session_callback new_session_ {};

    public:
        using ptr = ptr<hybrid_serve>;

        explicit hybrid_serve(hare::ptr<io::cycle> _cycle, std::string _name = "HARE_SERVE");
        virtual ~hybrid_serve();

        inline auto is_running() const -> bool { return started_; }
        inline void set_new_session(new_session_callback _new_session) { new_session_ = std::move(_new_session); }

        auto add_acceptor(const acceptor::ptr& _acceptor) -> bool;
        void remove_acceptor(util_socket_t _fd);

        auto exec(int32_t _thread_nbr) -> error;
        void exit();

    private:
        void active_acceptors();
        void new_session(util_socket_t _fd, host_address& _address, const timestamp& _time, util_socket_t _acceptor);
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SERVE_H_