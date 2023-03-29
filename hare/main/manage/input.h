#ifndef _HARE_MANAGE_INPUT_H_
#define _HARE_MANAGE_INPUT_H_

#include <hare/base/util/thread.h>
#include <hare/core/stream_serve.h>

#include <cstdint>
#include <list>
#include <tuple>

namespace hare {
namespace manage {

    class Input {
        
        hare::Thread input_thread_;

        std::list<std::tuple<int32_t, int8_t>> listen_ports_ {};

    public:
        static auto instance() -> Input&;

        ~Input();

        void init(int32_t argc, char** argv);

        auto initServe(core::StreamServe::Ptr& serve_ptr) -> bool;

    private:
        Input();

        void run();
    };

} // namespace manage
} // namespace hare

#endif // !_HARE_MANAGE_INPUT_H_