#include <hare/core/stream_serve.h>

#include <hare/base/logging.h>
#include <hare/core/rtmp/client.h>
#include <hare/net/hybrid_serve.h>
#include <hare/net/socket.h>
#include <hare/net/tcp_session.h>


namespace hare {
namespace core {

    stream_serve::stream_serve(io::cycle::REACTOR_TYPE _type)
        : main_cycle_(HARE_CHECK_NULL(new io::cycle(_type)))
    {
        serve_ = std::make_shared<net::hybrid_serve>(main_cycle_, "HARE-STREAM");
        serve_->set_new_session(std::bind(&stream_serve::new_session, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        LOG_TRACE() << "stream serve was created.";
    }

    stream_serve::~stream_serve()
    {
        serve_.reset();
        main_cycle_.reset();
        LOG_TRACE() << "stream serve is deleted.";
    }

    auto stream_serve::listen(int16_t _port, PROTOCOL_TYPE _type, int8_t _family) -> bool
    {
        net::TYPE type { net::TYPE_INVALID };

        if (_type == PROTOCOL_TYPE_RTMP) {
            type = net::TYPE_TCP;
        }

        if (type == net::TYPE_INVALID) {
            return false;
        }

        auto acc = std::make_shared<net::acceptor>(_family, type, _port, true);

        types_map_.insert(std::make_pair(acc->fd(), _type));

        return serve_->add_acceptor(acc);
    }

    void stream_serve::stop()
    {
        serve_->exit();
    }

    void stream_serve::run()
    {
        serve_->exec(static_cast<int32_t>(std::thread::hardware_concurrency()));
    }

    void stream_serve::new_session(ptr<net::session> _session, timestamp _time, const ptr<net::acceptor>& _acceptor)
    {
        LOG_INFO() << "New session[" << _session->name() << "] in " << _time.to_fmt() << " on " << _acceptor->fd();

        HARE_ASSERT(types_map_.find(_acceptor->fd()) != types_map_.end(), "unrecongize acceptor.");

        auto client_type = types_map_[_acceptor->fd()];
        switch (client_type) {
        case PROTOCOL_TYPE_RTMP:
        case PROTOCOL_TYPE_INVALID:
        default:
            break;
        }
    }

} // namespace core
} // namespace hare