#include <hare/core/stream_serve.h>

#include <hare/base/logging.h>
#include <hare/core/rtmp/client.h>
#include <hare/net/hybrid_serve.h>
#include <hare/net/tcp_session.h>

#include <map>
#include <mutex>

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

    void stream_serve::stop()
    {

    }

    void stream_serve::run()
    {
        serve_->exec(static_cast<int32_t>(std::thread::hardware_concurrency()));
    }

    void stream_serve::new_session(ptr<net::session> _session, timestamp _time, const ptr<net::acceptor>& _acceptor)
    {
        LOG_INFO() << "New session[" << _session->name()
                   << "] in " << _time.to_fmt();

        
    }

} // namespace core
} // namespace hare