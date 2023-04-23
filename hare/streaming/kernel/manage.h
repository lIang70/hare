#ifndef _HARE_STREAMING_MANAGE_H_
#define _HARE_STREAMING_MANAGE_H_

#include <hare/streaming/kernel/client.h>

#include <map>
#include <mutex>

namespace hare {
namespace net {
    class hybrid_serve;
    class session;
    class acceptor;
} // namespace net

namespace streaming {

    class manage {
        struct node;

        std::mutex node_mutex_ {};
        std::map<util_socket_t, ptr<struct node>> nodes_ {};

    public:
        manage();
        ~manage();

        auto add_node(PROTOCOL_TYPE _type, ptr<net::acceptor> _acceptor) -> bool;
        auto node_type(util_socket_t _accept_fd) -> PROTOCOL_TYPE;
        auto add_client(util_socket_t _accept_fd, const ptr<client>& _client) -> bool;
        void del_client(util_socket_t _accept_fd, util_socket_t _fd);

    };

} // namespace streaming
} // namespace hare

#endif // !_HARE_STREAMING_MANAGE_H_