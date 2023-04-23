#ifndef _HARE_STREAMING_CLIENT_RTMP_H_
#define _HARE_STREAMING_CLIENT_RTMP_H_

#include <hare/streaming/kernel/client.h>

namespace hare {
namespace streaming {

    class HARE_API client_rtmp : public client {
    public:
        explicit client_rtmp(util_socket_t _fd);
        ~client_rtmp() override = default;

        void process(io::buffer& buffer, const hare::ptr<net::session>& _session) override;
    };

} // namespace streaming
} // namespace hare

#endif // !_HARE_STREAMING_CLIENT_RTMP_H_