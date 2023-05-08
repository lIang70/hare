#include <hare/base/io/console.h>
#include <hare/base/io/cycle.h>
#include <hare/base/logging.h>
#include <hare/net/acceptor.h>
#include <hare/net/hybrid_serve.h>
#include <hare/net/tcp_session.h>
#include <hare/net/udp_session.h>

#include <map>

#include <sys/socket.h>

#define USAGE "echo_serve -p [port] [udp/tcp]"

using hare::net::acceptor;
using hare::net::session;
using hare::net::tcp_session;
using hare::net::udp_session;

static void new_session(const session::ptr& _ses, hare::timestamp _ts, const acceptor::ptr& _acc)
{
    if (_acc->type() == hare::net::TYPE_TCP) {
        _ses->set_connect_callback([=](const session::ptr& _session, uint8_t _event) {
            if ((_event & hare::net::SESSION_CONNECTED) != 0) {
                LOG_INFO() << "session[" << _session->name() << "] is connected.";
            }
            if ((_event & hare::net::SESSION_CLOSED) != 0) {
                LOG_INFO() << "session[" << _session->name() << "] is disconnected.";
            }
        });

        auto tsession = std::static_pointer_cast<tcp_session>(_ses);

        tsession->set_read_callback([](const hare::ptr<tcp_session>& _session, hare::io::buffer& _buffer, const hare::timestamp&) {
            _session->append(_buffer);
        });

        tsession->set_write_callback([=](const hare::ptr<tcp_session>& _session) {

        });

        tsession->set_high_water_callback([=](const hare::ptr<tcp_session>& _session) {
            LOG_ERROR() << "session[" << _session->name() << "] is going offline because it is no longer receiving data.";
            _session->force_close();
        });

        LOG_INFO() << "recv a new session[" << _ses->name() << "] at " << _ts.to_fmt(true) << " on acceptor=" << _acc->fd();
    } else if (_acc->type() == hare::net::TYPE_UDP) {
        _ses->set_connect_callback([=](const session::ptr& _session, uint8_t _event) {
            if ((_event & hare::net::SESSION_CONNECTED) != 0) {
                LOG_INFO() << "session[" << _session->name() << "] is connected.";
            }
            if ((_event & hare::net::SESSION_CLOSED) != 0) {
                LOG_INFO() << "session[" << _session->name() << "] is disconnected.";
            }
        });

        auto usession = std::static_pointer_cast<udp_session>(_ses);

        usession->set_read_callback([](const hare::ptr<udp_session>& _session, hare::io::buffer& _buffer, const hare::timestamp&) {
            _session->append(_buffer);
        });

        usession->set_write_callback([=](const hare::ptr<udp_session>& _session) {
            
        });

        LOG_INFO() << "recv a new session[" << _ses->name() << "] at " << _ts.to_fmt(true) << " on acceptor=" << _acc->fd();
    }
}

auto main(int32_t argc, char** argv) -> int32_t
{
    hare::logger::set_level(hare::log::LEVEL_TRACE);

    if (argc < 4) {
        LOG_FATAL() << USAGE;
    }

    hare::net::TYPE acceptor_type = hare::net::TYPE_INVALID;
    if (std::string(argv[3]) == "tcp") {
        acceptor_type = hare::net::TYPE_TCP;
    } else if (std::string(argv[3]) == "udp") {
        acceptor_type = hare::net::TYPE_UDP;
    }

    acceptor::ptr acc { new acceptor(AF_INET, acceptor_type, int16_t(std::stoi(std::string(argv[2])))) };

    hare::io::cycle::ptr main_cycle = std::make_shared<hare::io::cycle>(hare::io::cycle::REACTOR_TYPE_EPOLL);
    hare::net::hybrid_serve::ptr main_serve = std::make_shared<hare::net::hybrid_serve>(main_cycle, "ECHO");

    main_serve->set_new_session(new_session);
    main_serve->add_acceptor(acc);

    hare::io::console console;
    console.register_handle("quit", [=] {
        main_cycle->exit();
    });
    console.attach(main_cycle);

    LOG_INFO() << "========= ECHO serve start =========";

    auto ret = main_serve->exec(1);

    LOG_INFO() << "========= ECHO serve stop! =========";

    acc.reset();

    return (ret ? (0) : (-1));
}