#ifndef _HARE_NET_CORE_EVENT_H_
#define _HARE_NET_CORE_EVENT_H_

#include <hare/base/time/timestamp.h>
#include <hare/base/util/non_copyable.h>

#include <atomic>
#include <memory>

namespace hare {

    using EVENT = enum {
        /**
         * @brief The default flag.
         */
        EVENT_DEFAULT = 0x00,
        /**
         * @brief Wait for a socket or FD to become readable.
         */
        EVENT_READ = 0x01,
        /**
         * @brief Wait for a socket or FD to become writeable.
         */
        EVENT_WRITE = 0x02,
        /**
         * @brief Select edge-triggered behavior, if supported by the backend.
         */
        EVENT_ET = 0x04,
        /**
         * @brief Detects connection close events. You can use this to detect when a
         *   connection has been closed, without having to read all the pending data
         *   from a connection.
         */
        EVENT_CONNECTED = 0x08,
        EVENT_CLOSED = 0x10,
        EVENT_ERROR = 0x20
    };

namespace io {

    class cycle;
    class HARE_API event : public non_copyable
                         , public std::enable_shared_from_this<event> {
    public:
        enum Status : int32_t {
            NEW = -1,
            ADD = 1,
            DELETE = 2,
            STATUS_CNT
        };

    private:
        util_socket_t fd_ { -1 };
        int32_t index_ { Status::NEW };
        int32_t event_flags_ { EVENT_DEFAULT };
        int32_t revent_flags_ { EVENT_DEFAULT };
        wptr<cycle> owner_cycle_ { };

        std::atomic<bool> event_handle_ { false };
        std::atomic<bool> added_to_cycle_ { false };
        std::atomic<bool> tied_ { false };
        wptr<void> tie_object_ {};

    public:
        using ptr = ptr<event>;

        event(cycle* cycle, util_socket_t target_fd);
        virtual ~event();

        inline auto fd() const -> util_socket_t { return fd_; }
        inline auto owner_cycle() const -> hare::ptr<cycle> { return owner_cycle_.lock(); }
        inline void set_cycle(const hare::ptr<cycle>& _cycle) { owner_cycle_ = _cycle; }
        inline auto none_event() const -> bool { return event_flags_ == EVENT_DEFAULT; };

        inline auto flags() const -> int32_t { return event_flags_; }
        inline void set_flags(int32_t flags)
        {
            event_flags_ |= flags;
            active();
        }
        inline void clear_flags(int32_t flags)
        {
            event_flags_ &= ~flags;
            active();
        }
        inline auto check_flag(int32_t flags) const -> bool
        {
            return (event_flags_ & flags) != 0;
        }
        inline void clear_all()
        {
            event_flags_ = EVENT_DEFAULT;
            active();
        }

        auto flags_string() const -> std::string;
        auto rflags_string() const -> std::string;

        inline void set_rflags(int32_t flags) { revent_flags_ = flags; }

        void handle_event(timestamp& receive_time);

        /**
         * @brief Tie this channel to the owner object managed by shared_ptr,
         *   prevent the owner object being destroyed in handleEvent.
         */
        void tie(const hare::ptr<void>& object);
        auto tied_object() -> wptr<void> { return tie_object_; }

        void deactive();

    protected:
        void active();

        virtual void event_callback(int32_t events, const timestamp& receive_time) = 0;
    };

} // namespace io
} // namespace hare

#endif // !_HARE_NET_CORE_EVENT_H_