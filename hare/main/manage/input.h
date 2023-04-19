#ifndef _MANAGE_INPUT_H_
#define _MANAGE_INPUT_H_

#include <hare/core/stream_serve.h>

#include <list>
#include <tuple>

namespace manage {

class input {

    std::list<std::tuple<int32_t, hare::net::TYPE, int8_t>> listen_ports_ {};

public:
    static auto instance() -> input&;

    ~input();

    void init(int32_t argc, char** argv);

    auto init_serve(hare::core::stream_serve& serve_ptr) -> bool;

private:
    input();
};

} // namespace manage

#endif // !_MANAGE_INPUT_H_