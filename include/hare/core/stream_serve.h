#ifndef _HARE_CORE_STREAM_SERVE_H_
#define _HARE_CORE_STREAM_SERVE_H_

#include <hare/base/io/cycle.h>
#include <hare/net/socket.h>

namespace hare {
namespace net {
    class hybrid_serve;
    class session;
    class acceptor;
} // namespace net

namespace core {

    class HARE_API stream_serve : public non_copyable {
        ptr<io::cycle> main_cycle_ { nullptr };
        ptr<net::hybrid_serve> serve_ { nullptr };

    public:
        explicit stream_serve(io::cycle::REACTOR_TYPE _type);
        ~stream_serve();

        auto listen(uint16_t _port, hare::net::TYPE _type, int8_t _family) -> bool;

        void stop();
        void run();

    protected:
        void new_session(ptr<net::session> _session, timestamp _time, const ptr<net::acceptor>& _acceptor);
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_STREAM_SERVE_H_