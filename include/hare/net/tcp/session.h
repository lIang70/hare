/**
 * @file hare/net/tcp/session.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with session.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_TCP_SESSION_H_
#define _HARE_NET_TCP_SESSION_H_

#include <hare/base/io/event.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/util/any.h>
#include <hare/net/buffer.h>
#include <hare/net/socket.h>

namespace hare {
namespace net {
    namespace tcp {

        using SESSION = enum : std::uint8_t {
            SESSION_DEFAULT = io::EVENT_DEFAULT,
            SESSION_READ = io::EVENT_READ,
            SESSION_WRITE = io::EVENT_WRITE,
            SESSION_CLOSED = io::EVENT_CLOSED,
            SESSION_ERROR = 0x40,
            SESSION_CONNECTED = 0x80,
        };

        using STATE = enum : std::int8_t {
            STATE_CONNECTING = 0x00,
            STATE_CONNECTED,
            STATE_DISCONNECTING,
            STATE_DISCONNECTED
        };

        class client;
        class serve;

        HARE_CLASS_API
        class HARE_API session : public util::non_copyable
                               , public std::enable_shared_from_this<session> {
            hare::detail::impl* impl_ {};

        public:
            using ptr = ptr<session>;
            using connect_callback = std::function<void(const hare::ptr<session>&, std::uint8_t)>;
            using write_callback = std::function<void(const hare::ptr<session>&)>;
            using high_water_callback = std::function<void(const hare::ptr<session>&)>;
            using read_callback = std::function<void(const hare::ptr<session>&, buffer&, const timestamp&)>;
            using destroy = std::function<void()>;

            virtual ~session();

            auto name() const -> std::string;
            auto owner_cycle() const -> io::cycle*;
            auto local_address() const -> const host_address&;
            auto peer_address() const -> const host_address&;
            auto type() const -> TYPE;
            auto state() const -> STATE;
            auto fd() const -> util_socket_t;

            void set_connect_callback(connect_callback _connect);
            void set_high_water_mark(std::size_t _hwm);
            void set_read_callback(read_callback _read);
            void set_write_callback(write_callback _write);
            void set_high_water_callback(high_water_callback _high_water);

            void set_context(const util::any& context);
            auto get_context() const -> const util::any&;

            auto shutdown() -> error;
            auto force_close() -> error;

            void start_read();
            void stop_read();

            HARE_INLINE auto connected() const -> bool
            { return state() == STATE_CONNECTED; }

            auto append(buffer& _buffer) -> bool;
            auto send(const void* _bytes, std::size_t _length) -> bool;

            auto set_tcp_no_delay(bool _on) -> error;

        protected:
            session(io::cycle* _cycle,
                host_address _local_addr,
                std::string _name, std::uint8_t _family, util_socket_t _fd,
                host_address _peer_addr);

            void set_state(STATE _state);
            auto event() -> io::event::ptr&;
            auto socket() -> net::socket&;
            void set_destroy(destroy _destroy);

            void handle_callback(const io::event::ptr& _event, std::uint8_t _events, const timestamp& _receive_time);

            void handle_read(const timestamp&);
            void handle_write();
            void handle_close();
            void handle_error();

        private:
            void connect_established();

            friend class client;
            friend class serve;
        };

    } // namespace tcp
} // namespace net
} // namespace hare

#endif // _HARE_NET_TCP_SESSION_H_