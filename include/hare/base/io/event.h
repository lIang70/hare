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

    using Events = enum : std::uint8_t {
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
    class Event;
    template class HARE_API std::weak_ptr<Event>;
#endif

    class Cycle;
    HARE_CLASS_API
    class HARE_API Event : public util::NonCopyable
                         , public std::enable_shared_from_this<Event> {
        hare::detail::Impl* impl_ {};

    public:
        using Id = std::int64_t;
        using Callback = std::function<void(const Ptr<Event>&, std::uint8_t, const Timestamp&)>;

        Event(util_socket_t _fd, Callback _cb, std::uint8_t _events, std::int64_t _timeval);
        virtual ~Event();

        auto fd() const -> util_socket_t;
        auto events() const -> std::uint8_t;
        auto timeval() const -> std::int64_t;
        auto cycle() const -> Cycle*;
        auto id() const -> Id;

        // None of the following interfaces are thread-safe.
        void EnableRead();
        void DisableRead();
        auto Reading() -> bool;
        void EnableWrite();
        void DisableWrite();
        auto Writing() -> bool;
        void Deactivate();

        auto EventToString() const -> std::string;

        /**
         * @brief Tie this event to the owner object managed by shared_ptr,
         *   prevent the owner object being destroyed in handle_event.
         */
        void Tie(const hare::Ptr<void>& _obj);
        auto TiedObject() -> WPtr<void>;

    private:
        void HandleEvent(std::uint8_t _flag, Timestamp& _receive_time);
        void Active(Cycle* _cycle, Event::Id _id);
        void Reset();

        friend class Cycle;
    };

} // namespace io
} // namespace hare

#endif // _HARE_BASE_IO_EVENT_H_
