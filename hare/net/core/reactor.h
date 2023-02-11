#ifndef _HARE_NET_REACTOR_H_
#define _HARE_NET_REACTOR_H_

#include "hare/net/core/cycle.h"
#include <hare/base/detail/non_copyable.h>
#include <hare/base/timestamp.h>
#include <hare/net/util.h>

#include <map>
#include <string>

namespace hare {
namespace core {

    class Reactor : public NonCopyable {
        Cycle* owner_cycle_ { nullptr };

    protected:
        using EventMap = std::map<socket_t, Event*>;
        EventMap events_;

    public:
        static Reactor* createByType(const std::string& type, Cycle*);

        virtual ~Reactor() = default;

        //! @brief Polls the I/O events.
        //!  Must be called in the cycle thread.
        virtual Timestamp poll(int timeout_microseconds, Cycle::EventList& active_events) = 0;

        //! @brief Changes the interested I/O events.
        //!  Must be called in the cycle thread.
        virtual void updateEvent(Event* event) = 0;

        //! @brief Remove the event, when it destructs.
        //!  Must be called in the cycle thread.
        virtual void removeEvent(Event* event) = 0;

        //! @brief Detects whether the event is in the reactor.
        //!  Must be called in the cycle thread.
        virtual bool checkEvent(Event* event) const;

        inline void assertInLoopThread() const
        {
            owner_cycle_->assertInLoopThread();
        }

    protected:
        Reactor(Cycle* cycle)
            : owner_cycle_(cycle)
        {
        }
    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_REACTOR_H_