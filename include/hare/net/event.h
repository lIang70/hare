#ifndef _HARE_NET_EVENT_H_
#define _HARE_NET_EVENT_H_

#include <hare/base/timestamp.h>
#include <hare/net/util.h>

#include <memory>

namespace hare {

class Cycle;
class Event
    : public std::enable_shared_from_this<Event> {
public:
    friend class Cycle;

    enum Flags : int8_t {
        //! @brief The default flag.
        EV_DEFAULT = 0x00,
        //! @brief Indicates that a timeout has occurred.
        //!  It's not necessary to pass this flag to Event.
        EV_TIMEOUT = 0x01,
        //! @brief Wait for a socket or FD to become readable.
        EV_READ = 0x02,
        //! @brief Wait for a socket or FD to become writeable.
        EV_WRITE = 0x04,
        //! @brief Wait for a POSIX signal to be raised.
        EV_PERSIST = 0x08,
        //! @brief Select edge-triggered behavior, if supported by the backend.
        EV_ET = 0x10,
    };

private:
    struct Data;

    socket_t fd_ { -1 };
    Cycle* cycle_ { nullptr };
    std::shared_ptr<Data> d_ { nullptr };

public:
    Event();
    virtual ~Event();

    inline socket_t fd() const { return fd_; }
    inline Cycle* ownerCycle() const { return cycle_; }

    void setFlag(int8_t flags);
    void clearFlag(int8_t flags);
    void clearAllFlags();

    void deactive();

protected:
    void update();

    void handleEvent(Timestamp receive_time);
};

} // namespace hare

#endif // !_HARE_NET_EVENT_H_