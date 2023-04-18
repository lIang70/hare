#include <hare/base/io/console.h>
#include <hare/base/io/cycle.h>
#include <hare/base/logging.h>
#include <hare/net/hybrid_serve.h>

#include <map>

#define USAGE "echo_serve -p [port]"

using hare::net::acceptor;
using hare::net::session;

using all_session = std::map<hare::util_socket_t, session::ptr>;

static auto sessions() -> all_session&
{
    static std::map<hare::util_socket_t, session::ptr> s_session {};
    return s_session;
}

static void new_session(session::ptr _ses, hare::timestamp _ts, const acceptor::ptr& _acc)
{

    auto iter = sessions().find(_ses->fd());
    if (iter == sessions().end()) {
        _ses->set_connect_callback([=](const session::ptr& _session, uint8_t _event) {
            if ((_event & hare::net::SESSION_CONNECTED) != 0) {
                LOG_INFO() << "session[" << _session->name() << "] is connected.";
            }
            if ((_event & hare::net::SESSION_CLOSED) != 0) {
                LOG_INFO() << "session[" << _session->name() << "] is disconnected.";
                sessions().erase(_session->fd());
            }
        });

        _ses->set_read_callback([=](const session::ptr& _session, hare::io::buffer& _buffer, const hare::timestamp&) {
            _session->append(_buffer);
        });

        _ses->set_write_callback([=](const session::ptr& _session) {

        });

        _ses->set_high_water_callback([=](const session::ptr& _session) {
            LOG_ERROR() << "session[" << _session->name() << "] is going offline because it is no longer receiving data.";
            sessions().erase(_session->fd());
        });

        LOG_INFO() << "recv a new session[" << _ses->name() << "] at " << _ts.to_fmt(true) << " on acceptor=" << _acc->fd();
        sessions().insert(std::make_pair(_ses->fd(), std::move(_ses)));

    } else {
        LOG_ERROR() << "have the same fd for session[" << _ses->fd() << "]";
    }
}

auto main(int32_t argc, char** argv) -> int32_t
{
    hare::logger::set_level(hare::log::LEVEL_INFO);

    if (argc != 3) {
        LOG_FATAL() << USAGE;
    }

    acceptor::ptr acc { new acceptor(2, hare::net::TYPE_TCP, int16_t(std::stoi(std::string(argv[2])))) };

    hare::io::cycle::ptr main_cycle = std::make_shared<hare::io::cycle>(hare::io::cycle::REACTOR_TYPE_EPOLL);
    hare::net::hybrid_serve::ptr main_serve = std::make_shared<hare::net::hybrid_serve>(main_cycle, "ECHO");

    main_serve->set_new_session(new_session);
    main_serve->add_acceptor(acc);

    hare::io::console console;
    console.register_handle("quit", [=] { 
        for (auto& session : sessions()) {
            session.second->force_close();
        }
        sessions().clear();
        main_cycle->exit(); 
    });
    console.attach(main_cycle);

    LOG_INFO() << "========= ECHO serve start =========";

    auto ret = main_serve->exec(1);

    LOG_INFO() << "========= ECHO serve stop! =========";

    acc.reset();

    return (ret ? (0) : (-1));
}