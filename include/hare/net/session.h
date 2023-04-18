#ifndef _HARE_NET_SESSION_H_
#define _HARE_NET_SESSION_H_

#include <hare/base/io/buffer.h>
#include <hare/base/io/event.h>
#include <hare/base/time/timestamp.h>
#include <hare/net/socket.h>

namespace hare {
namespace net {

    using SESSION = enum : uint8_t {
        SESSION_DEFAULT = io::EVENT_DEFAULT,
        SESSION_READ = io::EVENT_READ,
        SESSION_WRITE = io::EVENT_WRITE,
        SESSION_CLOSED = io::EVENT_CLOSED,
        SESSION_ERROR = 0x40,
        SESSION_CONNECTED = 0x80,
    };

    using STATE = enum : int8_t {
        STATE_CONNECTING = 0x00,
        STATE_CONNECTED,
        STATE_DISCONNECTING,
        STATE_DISCONNECTED
    };

    static const size_t DEFAULT_HIGH_WATER = static_cast<size_t>(64 * 1024 * 1024);

    class HARE_API session : public non_copyable
                           , public std::enable_shared_from_this<session> {
        using connect_callback = std::function<void(const ptr<session>&, uint8_t)>;

        const std::string name_ {};
        io::cycle* cycle_ { nullptr };
        io::event::ptr event_ { nullptr };
        socket socket_;

        const host_address local_addr_ {};
        const host_address peer_addr_ {};

        io::buffer in_buffer_ {};
        io::buffer out_buffer_ {};
        size_t high_water_mark_ { DEFAULT_HIGH_WATER };

        bool reading_ { false };
        STATE state_ { STATE_CONNECTING };

        connect_callback connect_ {};

    public:
        using ptr = ptr<session>;
        using write_callback = std::function<void(const session::ptr&)>;
        using high_water_callback = std::function<void(const session::ptr&)>;
        using read_callback = std::function<void(const session::ptr&, io::buffer&, const timestamp&)>;

        virtual ~session();

        inline auto name() const -> std::string { return name_; }
        inline auto owner_cycle() const -> io::cycle* { return cycle_; }
        inline auto local_address() const -> const host_address& { return local_addr_; }
        inline auto peer_address() const -> const host_address& { return peer_addr_; }
        inline auto state() const -> STATE { return state_; }
        inline auto fd() const -> util_socket_t { return socket_.fd(); }
        inline auto high_water_mark() const -> size_t { return high_water_mark_; }
        inline void set_high_water_mark(size_t _high_water = DEFAULT_HIGH_WATER) { high_water_mark_ = _high_water; }
        inline void set_connect_callback(connect_callback _connect) { connect_ = std::move(_connect); }

        auto shutdown() -> error;
        auto force_close() -> error;

        void start_read();
        void stop_read();

        virtual auto append(io::buffer&) -> bool = 0;
        virtual auto send(void*, size_t) -> bool = 0;
        virtual void set_read_callback(read_callback) = 0;
        virtual void set_write_callback(write_callback) = 0;
        virtual void set_high_water_callback(high_water_callback) = 0;

    protected:
        session(io::cycle* _cycle, TYPE _type,
            host_address _local_addr,
            std::string _name, int8_t _family, util_socket_t _fd,
            host_address _peer_addr);

        inline void set_state(STATE _state) { state_ = _state; }
        inline auto in_buffer() -> io::buffer& { return in_buffer_; }
        inline auto out_buffer() -> io::buffer& { return out_buffer_; }
        inline auto event() -> io::event::ptr& { return event_; }
        inline auto socket() -> socket& { return socket_; }

        void handle_callback(const io::event::ptr& _event, uint8_t _events, const timestamp& _receive_time);

        virtual void handle_read(const timestamp&) = 0;
        virtual void handle_write() = 0;
        void handle_close();
        void handle_error();

    private:
        void connect_established();
        void connect_destroyed();

        friend class hybrid_serve;
    };

} // namespace net
} // namespace hare

#endif // ! _HARE_NET_SESSION_H_