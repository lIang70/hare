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

using hare::net::Acceptor;
using hare::net::TcpServe;
using hare::net::TcpSession;

static hare::Ptr<hare::log::Logger> server_logger {};

static void NewSession(const hare::Ptr<TcpSession>& _ses, hare::Timestamp _ts, const hare::Ptr<Acceptor>& _acc)
{
    _ses->SetConnectCallback([=](const hare::Ptr<TcpSession>& _session, std::uint8_t _event) {
        if ((_event & hare::net::SESSION_CONNECTED) != 0) {
            LOG_INFO(server_logger, "session[{}] is connected.", _session->Name());
        }
        if ((_event & hare::net::SESSION_CLOSED) != 0) {
            LOG_INFO(server_logger, "session[{}] is disconnected.", _session->Name());
        }
    });

    _ses->SetReadCallback([](const hare::Ptr<TcpSession>& _session, hare::net::Buffer& _buffer, const hare::Timestamp&) {
        _session->Append(_buffer);
    });

    _ses->SetWriteCallback([=](const hare::Ptr<TcpSession>& _session) {

    });

    _ses->SetHighWaterCallback([=](const hare::Ptr<TcpSession>& _session) {
        LOG_ERROR(server_logger, "session[{}] is going offline because it is no longer receiving data.", _session->Name());
        _session->ForceClose();
    });

    LOG_INFO(server_logger, "recv a new tcp-session[{}] at {} on acceptor={}.", _ses->Name(), _ts.ToFmt(true), _acc->Socket());
}

static void HandleMsg(std::uint8_t _msg_type, const std::string& _msg)
{
    if (_msg_type == hare::TRACE_MSG) {
        LOG_TRACE(server_logger, _msg);
    } else {
        LOG_ERROR(server_logger, _msg);
    }
}

auto main(std::int32_t argc, char** argv) -> std::int32_t
{
    using hare::log::Backend;
    using hare::log::FileBackendMT;
    using hare::log::STDBackendMT;
    using hare::log::detail::RotateFileBySize;

    if (argc < 4) {
        fmt::print(USAGE);
        return (0);
    }

    constexpr std::uint64_t file_size = static_cast<std::uint64_t>(64) * 1024 * 1024;

    auto tmp = hare::util::SystemDir();
    std::vector<hare::Ptr<Backend>> backends {
        std::make_shared<FileBackendMT<RotateFileBySize<file_size>>>(tmp + "/echo_serve"),
        STDBackendMT::Instance()
    };
    backends[0]->set_level(hare::log::LEVEL_TRACE);
    backends[1]->set_level(hare::log::LEVEL_INFO);

    server_logger = hare::log::Registry::create("echo_serve", backends.begin(), backends.end());
    server_logger->set_level(hare::log::LEVEL_TRACE);

    hare::RegisterLogHandler(HandleMsg);

    hare::Ptr<Acceptor> acceptor { new Acceptor(AF_INET, std::uint16_t(std::stoi(std::string(argv[2])))) };

#if defined(H_OS_WIN32)
    hare::io::cycle main_cycle(hare::io::cycle::REACTOR_TYPE_EPOLL);
#else
    hare::io::Cycle main_cycle(hare::io::Cycle::REACTOR_TYPE_EPOLL);
#endif

    hare::Ptr<TcpServe> main_serve = std::make_shared<TcpServe>(&main_cycle, "ECHO");

    main_serve->SetNewSession(NewSession);
    main_serve->AddAcceptor(acceptor);

    auto& console = hare::io::Console::Instance();
    console.RegisterHandle("quit", [&] {
        main_cycle.Exit();
    });
    console.Attach(&main_cycle);

    LOG_INFO(server_logger, "========= ECHO serve start =========");

    auto ret = main_serve->Exec(1);

    LOG_INFO(server_logger, "========= ECHO serve stop =========");

    acceptor.reset();

    server_logger->Flush();

    return (ret ? (0) : (-1));
}