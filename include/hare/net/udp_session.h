#ifndef _HARE_NET_UDP_SESSION_H_
#define _HARE_NET_UDP_SESSION_H_

#include <hare/net/session.h>

namespace hare {
namespace net {

    class HARE_API udp_session : public session {
        using write_callback = std::function<void(const hare::ptr<udp_session>&)>;
        using read_callback = std::function<void(const hare::ptr<udp_session>&, buffer&, const timestamp&)>;

        buffer out_buffer_ {};
        buffer in_buffer_ {};
        write_callback write_ {};
        read_callback read_ {};
        
    public:
        using ptr = hare::ptr<udp_session>;

        ~udp_session() override;

        inline void set_read_callback(read_callback _read) { read_ = std::move(_read); }
        inline void set_write_callback(write_callback _write) { write_ = std::move(_write); }

        auto append(buffer& _buffer) -> bool;
        auto send(const void* _bytes, size_t _length) -> bool override;

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

#endif // _HARE_NET_UDP_SESSION_H_