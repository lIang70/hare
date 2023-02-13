#ifndef _HARE_NET_CORE_EVENT_H_
#define _HARE_NET_CORE_EVENT_H_

#include <hare/base/timestamp.h>
#include <hare/net/util.h>

#include <atomic>
#include <memory>

namespace hare {
namespace core {

    class Cycle;
    class H_ALIGNAS(8) Event {
    public:
        enum Status : int32_t {
            NEW = -1,
            ADD = 1,
            DELETE = 2,
            STATUS_CNT
        };

    private:
        socket_t fd_ { -1 };
        int32_t index_ { Status::NEW };
        int32_t event_flags_ { EV_DEFAULT };
        int32_t revent_flags_ { EV_DEFAULT };
        Cycle* owner_cycle_ { nullptr };

        std::atomic<bool> event_handle_ { false };
        std::atomic<bool> added_to_cycle_ { false };

        std::atomic<bool> tied_ { false };
        std::weak_ptr<void> tie_object_;

    public:
        Event(Cycle* cycle, socket_t fd);
        virtual ~Event();

        inline socket_t fd() const { return fd_; }
        inline Cycle* ownerCycle() const { return owner_cycle_; }
        inline bool isNoneEvent() const { return event_flags_ == EV_DEFAULT; };
        inline int32_t index() { return index_; }
        inline void setIndex(int32_t index) { index_ = index; }

        inline int32_t flags() { return event_flags_; }
        inline void setFlags(int32_t flags)
        {
            event_flags_ |= flags;
            active();
        }
        inline void clearFlags(int32_t flags)
        {
            event_flags_ &= ~flags;
            active();
        }
        inline void clearAllFlags()
        {
            event_flags_ = EV_DEFAULT;
            active();
        }

        std::string flagsToString();
        std::string rflagsToString();

        inline void setRFlags(int32_t flags) { revent_flags_ = flags; }

        void handleEvent(Timestamp& receive_time);

        //! @brief Tie this channel to the owner object managed by shared_ptr,
        //!  prevent the owner object being destroyed in handleEvent.
        void tie(const std::shared_ptr<void>& object);

        void deactive();

    protected:
        void active();

        virtual void eventCallBack(int32_t events, Timestamp& receive_time) = 0;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_EVENT_H_