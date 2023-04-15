#ifndef _HARE_NET_CORE_EVENT_H_
#define _HARE_NET_CORE_EVENT_H_

#include <cstdint>
#include <hare/base/time/timestamp.h>
#include <hare/base/util/non_copyable.h>

#include <functional>

namespace hare {
namespace io {

    using EVENT = enum Event : uint8_t {
        EVENT_DEFAULT = 0x00,
        /**
        * @brief Indicates that a timeout has occurred.
        **/
        EVENT_TIMEOUT = 0x01,
        /**
        * @brief Wait for a socket or FD to become readable.
        **/
        EVENT_READ = 0x02,
        /**
        * @brief Wait for a socket or FD to become writeable.
        **/
        EVENT_WRITE = 0x04,
        /**
        * @brief Persistent event: won't get removed automatically when activated.
        *
        * When a persistent event with a timeout becomes activated, its timeout
        *   is reset to 0.
        **/
        EVENT_PERSIST	= 0x08,
        /**
        * @brief Select edge-triggered behavior, if supported by the backend.
        **/
        EVENT_ET = 0x10,
        /**
        * @brief Detects connection close events. You can use this to detect when a
        *   connection has been closed, without having to read all the pending data
        *   from a connection.
        *
        * Not all backends support EV_CLOSED.
        **/
        EVENT_CLOSED = 0x20,
    };

    class cycle;
    class HARE_API event : public non_copyable
                         , public std::enable_shared_from_this<event> {
    public:
        using ptr = ptr<event>;
        using callback = std::function<void(const event::ptr&, uint8_t events, const timestamp& receive_time)>;
        using id = int32_t;

    private:
        util_socket_t fd_ { -1 };
        uint8_t events_ { EVENT_DEFAULT };
        callback callback_ {};
        int64_t timeval_ { 0 };

        wptr<cycle> cycle_ {};
        id id_ { -1 };
        int64_t timeout_ { 0 };

        bool tied_ { false };
        wptr<void> tie_object_ {};

    public:
        event(util_socket_t _fd, callback _cb, uint8_t _flag, int64_t _timeval);
        virtual ~event();

        inline auto fd() const -> util_socket_t { return fd_; }
        inline auto events() const -> uint8_t { return events_; }
        inline auto timeval() const -> int64_t { return timeval_; }
        inline auto owner_cycle() const -> hare::ptr<cycle> { return cycle_.lock(); }
        inline auto event_id() const -> id { return id_; }

        void enable_read();
        void disenable_read();
        void enable_write();
        void disable_write();
        void deactivate();

        auto event_string() const -> std::string;

        /**
         * @brief Tie this event to the owner object managed by shared_ptr,
         *   prevent the owner object being destroyed in handle_event.
         */
        void tie(const hare::ptr<void>& _obj);
        auto tied_object() -> wptr<void> { return tie_object_; }

    private:
        void handle_event(uint8_t _flag, timestamp& _receive_time);

        friend class cycle;
    };

} // namespace io
} // namespace hare

#endif // !_HARE_NET_CORE_EVENT_H_