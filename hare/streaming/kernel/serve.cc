#include <hare/streaming/kernel/serve.h>

#include "hare/streaming/kernel/client_rtmp.h"
#include "hare/streaming/kernel/manage.h"
#include <hare/base/io/console.h>
#include <hare/base/logging.h>
#include <hare/net/acceptor.h>
#include <hare/net/hybrid_serve.h>

#include <memory>

namespace hare {
namespace streaming {

    void serve::start_logo()
    {
        ::fprintf(stdout, "%s\n", R"(       __    __   ______   _______   ________                        )");
        ::fprintf(stdout, "%s\n", R"(      |  \  |  \ /      \ |       \ |        \                       )");
        ::fprintf(stdout, "%s\n", R"(      | $$  | $$|  $$$$$$\| $$$$$$$\| $$$$$$$$                       )");
        ::fprintf(stdout, "%s\n", R"(      | $$__| $$| $$__| $$| $$__| $$| $$__                           )");
        ::fprintf(stdout, "%s\n", R"(      | $$    $$| $$    $$| $$    $$| $$  \                          )");
        ::fprintf(stdout, "%s\n", R"(      | $$$$$$$$| $$$$$$$$| $$$$$$$\| $$$$$                          )");
        ::fprintf(stdout, "%s\n", R"(      | $$  | $$| $$  | $$| $$  | $$| $$_____                        )");
        ::fprintf(stdout, "%s\n", R"(      | $$  | $$| $$  | $$| $$  | $$| $$     \                       )");
        ::fprintf(stdout, "%s\n", R"(       \$$   \$$ \$$   \$$ \$$   \$$ \$$$$$$$$                       )");
        ::fprintf(stdout, "%s\n", R"(            ___  ____  ____  ____    __    __  __  ____  _  _   ___  )");
        ::fprintf(stdout, "%s\n", R"(           / __)(_  _)(  _ \( ___)  /__\  (  \/  )(_  _)( \( ) / __) )");
        ::fprintf(stdout, "%s\n", R"(           \__ \  )(   )   / )__)  /(__)\  )    (  _)(_  )  ( ( (_-. )");
        ::fprintf(stdout, "%s\n", R"(           (___/ (__) (_)\_)(____)(__)(__)(_/\/\_)(____)(_)\_) \___/ )");
        ::fprintf(stdout, "%s\n", R"(=================== Welcome to HARE's streaming =====================)");
        ::fflush(stdout);
    }

    serve::serve(io::cycle::REACTOR_TYPE _type)
        : main_cycle_(HARE_CHECK_NULL(new io::cycle(_type)))
        , manage_(new manage)
    {
        serve_ = std::make_shared<net::hybrid_serve>(main_cycle_, "HARE-STREAM");
        serve_->set_new_session(std::bind(&serve::new_session, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        LOG_TRACE() << "stream serve was created.";
    }

    serve::~serve()
    {
        manage_.reset();
        serve_.reset();
        main_cycle_.reset();
        LOG_TRACE() << "stream serve is deleted.";
    }

    auto serve::listen(int16_t _port, PROTOCOL_TYPE _type, int8_t _family) -> bool
    {
        auto type { net::TYPE_INVALID };

        switch (_type) {
        case PROTOCOL_TYPE_RTMP:
            type = net::TYPE_TCP;
            break;
        default:
            break;
        }

        if (type == net::TYPE_INVALID) {
            return false;
        }

        auto acc = std::make_shared<net::acceptor>(_family, type, _port, true);
        HARE_ASSERT(acc, "an error occurred during the creation acceptor.");

        return serve_->add_acceptor(acc) ? manage_->add_node(_type, acc) : false;
    }

    auto serve::create_console() -> hare::ptr<io::console>
    {
        auto console = std::make_shared<io::console>();
        console->attach(main_cycle_);
        return console;
    }

    void serve::stop()
    {
        serve_->exit();
    }

    void serve::run()
    {
        serve_->exec(static_cast<int32_t>(std::thread::hardware_concurrency()));
    }

    void serve::new_session(const hare::ptr<net::session>& _session, timestamp _time, const hare::ptr<net::acceptor>& _acceptor)
    {
        H_UNUSED(_time);

        auto acceptor_fd = _acceptor->fd();
        auto session_fd = _session->fd();
        auto client_type = manage_->node_type(acceptor_fd);

        switch (client_type) {
        case PROTOCOL_TYPE_RTMP: {
            auto rtmp_client = hare::ptr<client>(new client_rtmp(session_fd));
            _session->set_read_callback([=](const net::session::ptr& _session, io::buffer& _buffer, const timestamp&) {
                rtmp_client->process(_buffer, _session);
            });
            _session->set_write_callback([](const net::session::ptr& _session) {
            });
            _session->set_high_water_callback([](const net::session::ptr& _session) {
            });
            _session->set_connect_callback([](const net::session::ptr& _session, uint8_t events) {
            });
            auto ret = manage_->add_client(acceptor_fd, rtmp_client);
            if (!ret) {
                _session->force_close();
            }
        } break;
        default:
            _session->force_close();
            break;
        }
    }

} // namespace streaming
} // namespace hare