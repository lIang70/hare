#ifndef _HARE_STREAMING_CLIENT_RTMP_H_
#define _HARE_STREAMING_CLIENT_RTMP_H_

#include <hare/streaming/kernel/client.h>

namespace hare {
namespace streaming {

    class HARE_API client_rtmp : public client {
    public:
        explicit client_rtmp(hare::ptr<net::session> _session);
        ~client_rtmp() override = default;

        void process(io::buffer& buffer, const timestamp& time) override;
    };

} // namespace streaming
} // namespace hare

#endif // !_HARE_STREAMING_CLIENT_RTMP_H_