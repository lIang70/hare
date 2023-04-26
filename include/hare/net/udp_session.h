#ifndef _HARE_NET_UDP_SESSION_H_
#define _HARE_NET_UDP_SESSION_H_

#include <hare/net/session.h>

namespace hare {
namespace net {

    class HARE_API udp_session : public session {
    public:
        using ptr = hare::ptr<udp_session>;

        ~udp_session() override;

        auto send(void* _bytes, size_t _length) -> bool override;

    protected:
        udp_session(io::cycle* _cycle,
            host_address _local_addr,
            std::string _name, int8_t _family, util_socket_t _fd,
            host_address _peer_addr);

        void handle_read(const timestamp& _time) override;
        void handle_write() override;

        friend class hybrid_serve;
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_UDP_SESSION_H_