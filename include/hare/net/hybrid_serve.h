/**
 * @file hare/net/hybrid_serve.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with hybrid_serve.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_HYBRID_SERVE_H_
#define _HARE_NET_HYBRID_SERVE_H_

#include <hare/net/session.h>

#include <map>

namespace hare {
namespace net {

    class io_pool;
    class acceptor;
    HARE_CLASS_API
    class HARE_API hybrid_serve : public util::non_copyable {
        using new_session_callback = std::function<void(const session::ptr&, const timestamp&, const hare::ptr<acceptor>&)>;

        std::string name_ {};

        // the acceptor loop
        ptr<io::cycle> cycle_ {};
        ptr<io_pool> io_pool_ {};
        uint64_t session_id_ { 0 };
        bool started_ { false };

        new_session_callback new_session_ {};

    public:
        using ptr = ptr<hybrid_serve>;

        explicit hybrid_serve(hare::ptr<io::cycle> _cycle, std::string _name = "HARE_SERVE");
        virtual ~hybrid_serve();

        HARE_INLINE auto main_cycle() const -> hare::ptr<io::cycle> { return cycle_; }
        HARE_INLINE auto is_running() const -> bool { return started_; }
        HARE_INLINE void set_new_session(new_session_callback _new_session) { new_session_ = std::move(_new_session); }

        auto add_acceptor(const hare::ptr<acceptor>& _acceptor) -> bool;

        auto exec(std::int32_t _thread_nbr) -> error;
        void exit();

    private:
        void new_session(util_socket_t _fd, host_address& _address, const timestamp& _time, acceptor* _acceptor);
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_TCP_SERVE_H_