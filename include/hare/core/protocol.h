#ifndef _HARE_CORE_PROTOCOL_H_
#define _HARE_CORE_PROTOCOL_H_

#include <cinttypes>
#include <memory>

namespace hare {
namespace core {

    class Protocol {
    public:
        using Ptr = std::shared_ptr<Protocol>;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_CORE_PROTOCOL_H_