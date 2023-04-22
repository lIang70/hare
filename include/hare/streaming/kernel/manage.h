#ifndef _HARE_STREAMING_MANAGE_H_
#define _HARE_STREAMING_MANAGE_H_

#include <hare/streaming/kernel/client.h>

#include <map>

namespace hare {
namespace net {
    class hybrid_serve;
    class session;
    class acceptor;
} // namespace net

namespace streaming {

    class HARE_API manage {
        struct node;

        std::map<util_socket_t, ptr<struct node>> nodes_ {};

    public:
        manage();
        ~manage();

        auto add_node(PROTOCOL_TYPE _type, ptr<net::acceptor> _acceptor) -> bool;
        auto node_type(util_socket_t _fd) -> PROTOCOL_TYPE;

    };

} // namespace streaming
} // namespace hare

#endif // !_HARE_STREAMING_MANAGE_H_