#ifndef _HARE_NET_CORE_EVENT_H_
#define _HARE_NET_CORE_EVENT_H_

#include <hare/base/timestamp.h>
#include <hare/net/util.h>

#include <memory>

namespace hare {
namespace core {

    class Cycle;
    class Event
        : public std::enable_shared_from_this<Event> {
    public:
        friend class Cycle;

        enum class Status : int8_t {
            NEW = 0,
            ADD = 1,
            DELETE = 2,
            STATUS_CNT
        };

    private:
        socket_t fd_ { -1 };
        Status status_ { Status::NEW };
        int32_t event_flags_ { EV_DEFAULT };
        int32_t revent_flags_ { EV_DEFAULT };
        Cycle* cycle_ { nullptr };

    public:
        virtual ~Event();

        inline socket_t fd() const { return fd_; }
        inline Cycle* ownerCycle() const { return cycle_; }
        inline bool isNoneEvent() const { return event_flags_ == EV_DEFAULT; };
        inline Status status() { return status_; }
        inline void setStatus(Status status) { status_ = status; }

        inline int32_t flags() { return event_flags_; }
        inline void setFlags(int32_t flags)
        {
            event_flags_ |= flags;
            update();
        }
        inline void clearFlags(int32_t flags)
        {
            event_flags_ &= ~flags;
            update();
        }
        inline void clearAllFlags()
        {
            event_flags_ = EV_DEFAULT;
            update();
        }

        std::string flagsToString();

        inline void setRFlags(int32_t flags) { revent_flags_ = flags; }

        void deactive();

    protected:
        void update();
        void handleEvent(Timestamp receive_time);

    private:
        Event() = default;

    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_EVENT_H_