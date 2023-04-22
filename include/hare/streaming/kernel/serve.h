#ifndef _HARE_STREAMING_SERVE_H_
#define _HARE_STREAMING_SERVE_H_

#include <hare/base/io/cycle.h>
#include <hare/streaming/protocol/protocol.h>

namespace hare {
namespace net {
    class hybrid_serve;
    class session;
    class acceptor;
} // namespace net

namespace streaming {

    class manage;
    class HARE_API serve : public non_copyable {
        ptr<io::cycle> main_cycle_ { nullptr };
        ptr<net::hybrid_serve> serve_ { nullptr };
        ptr<manage> manage_ { nullptr };

    public:
        static void start_logo();

        explicit serve(io::cycle::REACTOR_TYPE _type);
        ~serve();

        auto listen(int16_t _port, PROTOCOL_TYPE _type, int8_t _family) -> bool;

        void stop();
        void run();

    protected:
        void new_session(ptr<net::session> _session, timestamp _time, const ptr<net::acceptor>& _acceptor);
    };

} // namespace streaming
} // namespace hare

#endif // !_HARE_STREAMING_SERVE_H_