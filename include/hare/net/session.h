/**
 * @file hare/net/session.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with session.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_SESSION_H_
#define _HARE_NET_SESSION_H_

#include <hare/base/io/event.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/util/any.h>
#include <hare/net/buffer.h>
#include <hare/net/socket.h>

namespace hare {
namespace net {

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

    HARE_CLASS_API
    class HARE_API session : public util::non_copyable, public std::enable_shared_from_this<session> {
        hare::detail::impl* impl_ {};

    public:
        using connect_callback = std::function<void(const ptr<session>&, std::uint8_t)>;
        using destroy = std::function<void()>;
        using ptr = ptr<session>;

        virtual ~session();

        auto name() const -> std::string;
        auto owner_cycle() const -> io::cycle*;
        auto local_address() const -> const host_address&;
        auto peer_address() const -> const host_address&;
        auto state() const -> STATE;
        auto fd() const -> util_socket_t;

        auto connected() const -> bool;

        void set_connect_callback(connect_callback _connect);

        void set_context(const util::any& context);
        auto get_context() const -> const util::any&;

        auto shutdown() -> error;
        auto force_close() -> error;

        void start_read();
        void stop_read();

        virtual auto send(const void*, std::size_t) -> bool = 0;

    protected:
        session(io::cycle* _cycle, TYPE _type,
            host_address _local_addr,
            std::string _name, std::uint8_t _family, util_socket_t _fd,
            host_address _peer_addr);

        void set_state(STATE _state);
        auto event() -> io::event::ptr&;
        auto socket() -> net::socket&;
        void set_destroy(destroy _destroy);

        void handle_callback(const io::event::ptr& _event, std::uint8_t _events, const timestamp& _receive_time);

        virtual void handle_read(const timestamp&) = 0;
        virtual void handle_write() = 0;
        void handle_close();
        void handle_error();

    private:
        void connect_established();

        friend class hybrid_serve;
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_SESSION_H_