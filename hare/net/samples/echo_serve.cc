#include <hare/base/io/console.h>
#include <hare/base/io/cycle.h>
#include <hare/base/util/system.h>
#include <hare/log/backends/file_backend.h>
#include <hare/log/backends/std_backend.h>
#include <hare/log/registry.h>
#include <hare/net/tcp/acceptor.h>
#include <hare/net/tcp/serve.h>
#include <hare/net/tcp/session.h>

#include <map>

#if defined(H_OS_WIN32)
#include <WinSock2.h>
#else
#include <sys/socket.h>
#endif

#define USAGE "echo_serve -p [port]" HARE_EOL

using hare::net::tcp::acceptor;
using hare::net::tcp::session;
using hare::net::tcp::serve;

static hare::ptr<hare::log::logger> server_logger {};

static void new_session(const session::ptr& _ses, hare::timestamp _ts, const acceptor::ptr& _acc)
{
    _ses->set_connect_callback([=](const session::ptr& _session, std::uint8_t _event) {
        if ((_event & hare::net::tcp::SESSION_CONNECTED) != 0) {
            LOG_INFO(server_logger, "session[{}] is connected.", _session->name());
        }
        if ((_event & hare::net::tcp::SESSION_CLOSED) != 0) {
            LOG_INFO(server_logger, "session[{}] is disconnected.", _session->name());
        }
    });

    _ses->set_read_callback([](const hare::ptr<session>& _session, hare::net::buffer& _buffer, const hare::timestamp&) {
        _session->append(_buffer);
    });

    _ses->set_write_callback([=](const hare::ptr<session>& _session) {

    });

    _ses->set_high_water_callback([=](const hare::ptr<session>& _session) {
        LOG_ERROR(server_logger, "session[{}] is going offline because it is no longer receiving data.", _session->name());
        _session->force_close();
    });

    LOG_INFO(server_logger, "recv a new tcp-session[{}] at {} on acceptor={}.", _ses->name(), _ts.to_fmt(true), _acc->fd());
}

static void handle_msg(std::uint8_t _msg_type, const std::string& _msg) {
    if (_msg_type == hare::TRACE_MSG) {
        LOG_TRACE(server_logger, _msg);
    } else {
        LOG_ERROR(server_logger, _msg);
    }
}

auto main(std::int32_t argc, char** argv) -> std::int32_t
{
    using hare::log::backend;
    using hare::log::file_backend_mt;
    using hare::log::std_backend_mt;
    using hare::log::detail::rotate_file;

    if (argc < 4) {
        fmt::print(USAGE);
        return (0);
    }

    constexpr std::uint64_t file_size = static_cast<std::uint64_t>(64) * 1024 * 1024;

    auto tmp = hare::util::system_dir();
    std::vector<hare::ptr<backend>> backends {
        std::make_shared<file_backend_mt<rotate_file<file_size>>>(tmp + "/echo_serve"),
        std_backend_mt::instance()
    };
    backends[0]->set_level(hare::log::LEVEL_TRACE);
    backends[1]->set_level(hare::log::LEVEL_INFO);

    server_logger = hare::log::registry::create("echo_serve", backends.begin(), backends.end());
    server_logger->set_level(hare::log::LEVEL_TRACE);

    hare::register_msg_handler(handle_msg);

    acceptor::ptr acc { new acceptor(AF_INET, std::uint16_t(std::stoi(std::string(argv[2])))) };

#if defined(H_OS_WIN32)
    hare::io::cycle main_cycle(hare::io::cycle::REACTOR_TYPE_EPOLL);
#else
    hare::io::cycle main_cycle(hare::io::cycle::REACTOR_TYPE_EPOLL);
#endif
    
    serve::ptr main_serve = std::make_shared<serve>(&main_cycle, "ECHO");

    main_serve->set_new_session(new_session);
    main_serve->add_acceptor(acc);

    auto& console = hare::io::console::instance();
    console.register_handle("quit", [&] {
        main_cycle.exit();
    });
    console.attach(&main_cycle);

    LOG_INFO(server_logger, "========= ECHO serve start =========");

    auto ret = main_serve->exec(1);

    LOG_INFO(server_logger, "========= ECHO serve stop =========");

    acc.reset();

    server_logger->flush();

    return (ret ? (0) : (-1));
}