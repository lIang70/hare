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

#include <hare/net/tcp/session.h>

namespace hare {
namespace net {
    namespace tcp {

        class acceptor;

        HARE_CLASS_API
        class HARE_API serve : public util::non_copyable {
            hare::detail::impl* impl_ {};

        public:
            using new_session_callback = std::function<void(const session::ptr&, const timestamp&, const hare::ptr<acceptor>&)>;
            using ptr = ptr<serve>;

            explicit serve(io::cycle* _cycle, std::string _name = "HARE_SERVE");
            virtual ~serve();

            auto main_cycle() const -> io::cycle*;
            auto is_running() const -> bool;
            void set_new_session(new_session_callback _new_session);

            auto add_acceptor(const hare::ptr<acceptor>& _acceptor) -> bool;

            auto exec(std::int32_t _thread_nbr) -> error;
            void exit();

        private:
            void new_session(util_socket_t _fd, host_address& _address, const timestamp& _time, acceptor* _acceptor);
        };

    } // namespace tcp
} // namespace net
} // namespace hare

#endif // _HARE_NET_TCP_SERVE_H_