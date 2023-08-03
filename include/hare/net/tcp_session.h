/**
 * @file hare/net/tcp_session.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with tcp_session.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_TCP_SESSION_H_
#define _HARE_NET_TCP_SESSION_H_

#include <hare/net/session.h>

#include <mutex>

#define HARE_DEFAULT_HIGH_WATER (64UL * 1024 * 1024)

namespace hare {
namespace net {

    HARE_CLASS_API
    class HARE_API tcp_session : public session {
        hare::detail::impl* impl_ {};

    public:
        using ptr = hare::ptr<tcp_session>;
        using write_callback = std::function<void(const hare::ptr<tcp_session>&)>;
        using high_water_callback = std::function<void(const hare::ptr<tcp_session>&)>;
        using read_callback = std::function<void(const hare::ptr<tcp_session>&, buffer&, const timestamp&)>;

        ~tcp_session() override;

        void set_high_water_mark(size_t _hwm);
        void set_read_callback(read_callback _read);
        void set_write_callback(write_callback _write);
        void set_high_water_callback(high_water_callback _high_water);

        auto append(buffer& _buffer) -> bool;
        auto send(const void* _bytes, size_t _length) -> bool override;

        auto set_tcp_no_delay(bool _on) -> error;

    protected:
        tcp_session(io::cycle* _cycle,
            host_address _local_addr,
            std::string _name, std::uint8_t _family, util_socket_t _fd,
            host_address _peer_addr);

        void handle_read(const timestamp& _time) override;
        void handle_write() override;

        friend class net::hybrid_client;
        friend class net::hybrid_serve;
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_TCP_SESSION_H_