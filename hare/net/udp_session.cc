#include <hare/net/udp_session.h>

#include <hare/base/logging.h>

namespace hare {
namespace net {

    udp_session::~udp_session() = default;

    auto udp_session::send(void* _bytes, size_t _length) -> bool
    {
        if (state() == STATE_CONNECTED) {
            
            return true;
        }
        return false;
    }

    udp_session::udp_session(io::cycle* _cycle,
        host_address _local_addr,
        std::string _name, int8_t _family, util_socket_t _fd,
        host_address _peer_addr)
        : session(HARE_CHECK_NULL(_cycle), TYPE_TCP,
            std::move(_local_addr),
            std::move(_name), _family, _fd,
            std::move(_peer_addr))
    {
    }

    void udp_session::handle_read(const timestamp& _time)
    {
        
    }

    void udp_session::handle_write()
    {
    }

} // namespace net
} // namespace hare