#ifndef _HARE_NET_CORE_EVENT_H_
#define _HARE_NET_CORE_EVENT_H_

#include <hare/base/time/timestamp.h>
#include <hare/net/util.h>

#include <atomic>
#include <memory>

namespace hare {
namespace core {

    class Cycle;
    class Event {
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
        int32_t event_flags_ { net::EVENT_DEFAULT };
        int32_t revent_flags_ { net::EVENT_DEFAULT };
        Cycle* owner_cycle_ { nullptr };

        std::atomic<bool> event_handle_ { false };
        std::atomic<bool> added_to_cycle_ { false };

        std::atomic<bool> tied_ { false };
        std::weak_ptr<void> tie_object_;

    public:
        using Ptr = std::shared_ptr<Event>;

        Event(Cycle* cycle, util_socket_t target_fd);
        virtual ~Event();

        inline auto fd() const -> util_socket_t { return fd_; }
        inline auto ownerCycle() const -> Cycle* { return owner_cycle_; }
        inline auto isNoneEvent() const -> bool { return event_flags_ == net::EVENT_DEFAULT; };
        inline auto index() const -> int32_t { return index_; }
        inline void setIndex(int32_t index) { index_ = index; }

        inline auto flags() const -> int32_t { return event_flags_; }
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
        inline auto checkFlag(int32_t flags) const -> bool
        {
            return (event_flags_ & flags) != 0;
        }
        inline void clearAllFlags()
        {
            event_flags_ = net::EVENT_DEFAULT;
            active();
        }

        auto flagsToString() const -> std::string;
        auto rflagsToString() const -> std::string;

        inline void setRFlags(int32_t flags) { revent_flags_ = flags; }

        void handleEvent(Timestamp& receive_time);

        //! @brief Tie this channel to the owner object managed by shared_ptr,
        //!  prevent the owner object being destroyed in handleEvent.
        void tie(const std::shared_ptr<void>& object);
        auto tiedObject() -> std::weak_ptr<void> { return tie_object_; }

        void deactive();

    protected:
        void active();

        virtual void eventCallBack(int32_t events, const Timestamp& receive_time) = 0;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_CORE_EVENT_H_