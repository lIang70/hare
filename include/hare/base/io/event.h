/**
 * @file hare/base/io/event.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with event.h
 * @version 0.1-beta
 * @date 2023-04-14
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_IO_EVENT_H_
#define _HARE_BASE_IO_EVENT_H_

#include <hare/base/time/timestamp.h>
#include <hare/base/util/non_copyable.h>

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
         *   When a persistent event with a timeout becomes activated, its timeout
         *   is reset to 0.
         **/
        EVENT_PERSIST = 0x08,
        /**
         * @brief Select edge-triggered behavior, if supported by the backend.
         **/
        EVENT_ET = 0x10,
        /**
         * @brief Detects connection close events. You can use this to detect when a
         *   connection has been closed, without having to read all the pending data
         *   from a connection.
         *
         *   Not all backends support EV_CLOSED.
         **/
        EVENT_CLOSED = 0x20,
    };

#if defined(HARE_SHARED) && defined(H_OS_WIN)
    class event;
    template class __declspec(dllexport) std::weak_ptr<event>;
#endif

    class cycle;
    HARE_CLASS_API
    class HARE_API event : public util::non_copyable
                         , public std::enable_shared_from_this<event> {
        detail::impl* impl_ {};

    public:
        using id = int32_t;
        using ptr = ptr<event>;
        using callback = std::function<void(const event::ptr&, uint8_t, const timestamp&)>;

        event(util_socket_t _fd, callback _cb, uint8_t _events, int64_t _timeval);
        virtual ~event();

        auto fd() const -> util_socket_t;
        auto events() const -> std::uint8_t;
        auto timeval() const -> std::int64_t;
        auto owner_cycle() const -> cycle*;
        auto event_id() const -> id;

        // None of the following interfaces are thread-safe.
        void enable_read();
        void disable_read();
        auto reading() -> bool;
        void enable_write();
        void disable_write();
        auto writing() -> bool;
        void deactivate();

        auto event_string() const -> std::string;

        /**
         * @brief Tie this event to the owner object managed by shared_ptr,
         *   prevent the owner object being destroyed in handle_event.
         */
        void tie(const hare::ptr<void>& _obj);
        auto tied_object() -> wptr<void>;

    private:
        void handle_event(uint8_t _flag, timestamp& _receive_time);
        void active(cycle* _cycle, event::id _id);
        void reset();

        friend class cycle;
    };

} // namespace io
} // namespace hare

#endif // _HARE_BASE_IO_EVENT_H_
