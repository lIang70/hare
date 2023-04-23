#ifndef _HARE_STREAMING_SERVE_H_
#define _HARE_STREAMING_SERVE_H_

#include <hare/base/io/cycle.h>
#include <hare/streaming/protocol/protocol.h>

namespace hare {
namespace io {
    class console;
} // namespace io

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
        using ptr = ptr<serve>;

        static void start_logo();

        explicit serve(io::cycle::REACTOR_TYPE _type);
        ~serve();

        auto listen(int16_t _port, PROTOCOL_TYPE _type, int8_t _family) -> bool;

        auto create_console() -> hare::ptr<io::console>;

        void stop();
        void run();

    protected:
        void new_session(const hare::ptr<net::session>& _session, timestamp _time, const hare::ptr<net::acceptor>& _acceptor);


    };

} // namespace streaming
} // namespace hare

#endif // !_HARE_STREAMING_SERVE_H_