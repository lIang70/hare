#ifndef _HARE_NET_TCP_SESSION_H_
#define _HARE_NET_TCP_SESSION_H_

#include <hare/net/session.h>

#include <mutex>

namespace hare {
namespace net {

    class HARE_API tcp_session : public session {
        std::mutex write_mutex_ {};

        write_callback write_ {};
        high_water_callback high_water_ {};
        read_callback read_ {};

    public:
        using ptr = hare::ptr<tcp_session>;

        ~tcp_session() override;

        inline void set_read_callback(read_callback _read) override { read_ = std::move(_read); }
        inline void set_write_callback(write_callback _write) override { write_ = std::move(_write); }
        inline void set_high_water_callback(high_water_callback _high_water) override { high_water_ = std::move(_high_water); }

        auto append(io::buffer& _buffer) -> bool override;
        auto send(void* _bytes, size_t _length) -> bool override;

        auto set_tcp_no_delay(bool _on) -> error;

    protected:
        tcp_session(io::cycle* _cycle,
            host_address _local_addr,
            std::string _name, int8_t _family, util_socket_t _fd,
            host_address _peer_addr);

        void handle_read(const timestamp& _time) override;
        void handle_write() override;

        friend class hybrid_serve;
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SESSION_H_