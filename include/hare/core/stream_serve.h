#ifndef _HARE_CORE_STREAM_SERVE_H_
#define _HARE_CORE_STREAM_SERVE_H_

#include <hare/base/io/cycle.h>
#include <hare/core/protocol.h>

#include <map>

namespace hare {
namespace net {
    class hybrid_serve;
    class session;
    class acceptor;
} // namespace net

namespace core {

    class stream_client;
    class HARE_API stream_serve : public non_copyable {
        struct HARE_API SERVE_ITEM {
            std::map<util_socket_t, ptr<stream_client>> clients_ {};
        };

        ptr<io::cycle> main_cycle_ { nullptr };
        ptr<net::hybrid_serve> serve_ { nullptr };

        std::map<util_socket_t, PROTOCOL_TYPE> types_map_ {};

    public:
        explicit stream_serve(io::cycle::REACTOR_TYPE _type);
        ~stream_serve();

        auto listen(int16_t _port, PROTOCOL_TYPE _type, int8_t _family) -> bool;

        void stop();
        void run();

    protected:
        void new_session(ptr<net::session> _session, timestamp _time, const ptr<net::acceptor>& _acceptor);
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_STREAM_SERVE_H_