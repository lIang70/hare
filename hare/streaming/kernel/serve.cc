#include <hare/streaming/kernel/serve.h>

#include <hare/base/logging.h>
#include <hare/net/hybrid_serve.h>
#include <hare/streaming/kernel/manage.h>

namespace hare {
namespace streaming {

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
        net::TYPE type { net::TYPE_INVALID };

        if (_type == PROTOCOL_TYPE_RTMP) {
            type = net::TYPE_TCP;
        }

        if (type == net::TYPE_INVALID) {
            return false;
        }

        auto acc = std::make_shared<net::acceptor>(_family, type, _port, true);
        auto ret = serve_->add_acceptor(acc);

        if (ret) {
            manage_->add_node(_type, acc);
        }

        return ret;
    }

    void serve::stop()
    {
        serve_->exit();
    }

    void serve::run()
    {
        serve_->exec(static_cast<int32_t>(std::thread::hardware_concurrency()));
    }

    void serve::new_session(ptr<net::session> _session, timestamp _time, const ptr<net::acceptor>& _acceptor)
    {
        HARE_ASSERT(types_map_.find(_acceptor->fd()) != types_map_.end(), "unrecongize acceptor.");
        H_UNUSED(_time);

        auto client_type = types_map_[_acceptor->fd()];
        switch (client_type) {
        case PROTOCOL_TYPE_RTMP:
            break;
        default:
            _session->force_close();
            break;
        }
    }

} // namespace streaming
} // namespace hare