#ifndef _HARE_CORE_RTMP_CLIENT_H_
#define _HARE_CORE_RTMP_CLIENT_H_

#include <hare/core/stream_client.h>

namespace hare {
namespace core {

    class HARE_API rtmp_client : public stream_client {
    public:
        explicit rtmp_client(hare::ptr<net::session> _session);
        ~rtmp_client() override = default;

        void process(io::buffer& buffer, const timestamp& time) override;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_RTMP_CLIENT_H_