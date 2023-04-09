#ifndef _HARE_CORE_RTMP_CLIENT_H_
#define _HARE_CORE_RTMP_CLIENT_H_

#include <hare/core/stream_client.h>

namespace hare {
namespace core {

    class HARE_API RTMPClient : public StreamClient {

    public:
        RTMPClient();

        void process(net::Buffer& buffer, const Timestamp& time) override;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_RTMP_CLIENT_H_