#include <hare/streaming/kernel/manage.h>



namespace hare {
namespace streaming {

    struct manage::node {
        ptr<net::acceptor> acceptor { nullptr };
    };


} // namespace streaming
} // namespace hare