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
    class HARE_API session : public util::non_copyable
                           , public std::enable_shared_from_this<session> {
        using connect_callback = std::function<void(const ptr<session>&, std::uint8_t)>;
        using destroy = std::function<void()>;

        const std::string name_ {};
        io::cycle* cycle_ { nullptr };
        io::event::ptr event_ { nullptr };
        socket socket_;

        const host_address local_addr_ {};
        const host_address peer_addr_ {};

        bool reading_ { false };
        STATE state_ { STATE_CONNECTING };

        connect_callback connect_ {};
        destroy destroy_ {};

        util::any any_ctx_ {};

    public:
        using ptr = ptr<session>;

        virtual ~session();

        HARE_INLINE auto name() const -> std::string { return name_; }
        HARE_INLINE auto owner_cycle() const -> io::cycle* { return cycle_; }
        HARE_INLINE auto local_address() const -> const host_address& { return local_addr_; }
        HARE_INLINE auto peer_address() const -> const host_address& { return peer_addr_; }
        HARE_INLINE auto state() const -> STATE { return state_; }
        HARE_INLINE auto fd() const -> util_socket_t { return socket_.fd(); }

        HARE_INLINE void set_connect_callback(connect_callback _connect) { connect_ = std::move(_connect); }

        HARE_INLINE void set_context(const util::any& context) { any_ctx_ = context; }
        HARE_INLINE auto get_context() const -> const util::any& { return any_ctx_; }

        auto shutdown() -> error;
        auto force_close() -> error;

        void start_read();
        void stop_read();

        virtual auto send(const void*, std::size_t) -> bool = 0;

    protected:
        session(io::cycle* _cycle, TYPE _type,
            host_address _local_addr,
            std::string _name, std::int8_t _family, util_socket_t _fd,
            host_address _peer_addr);

        HARE_INLINE void set_state(STATE _state) { state_ = _state; }
        HARE_INLINE auto event() -> io::event::ptr& { return event_; }
        HARE_INLINE auto socket() -> socket& { return socket_; }

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