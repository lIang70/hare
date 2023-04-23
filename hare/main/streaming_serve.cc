#include <hare/base/io/console.h>
#include <hare/base/log/async.h>
#include <hare/base/logging.h>
#include <hare/base/util/system_info.h>
#include <hare/hare-config.h>
#include <hare/streaming/kernel/serve.h>

#ifdef HARE__HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HARE__HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HARE__HAVE_UNISTD_H
#include <unistd.h>
#endif

#define DEFAULT_DIR_MODE (0755)
#define DEFAULT_ROLL_SIZE (20 * 1024 * 1024) // 20MB
#define EAST_OF_UTC (8 * 3600) // 8-hours

namespace detail {

static void help()
{
    ::fprintf(stdout, "Usage\n\n");
    ::fprintf(stdout, "  hare [options]\n\n");
    ::fprintf(stdout, "Options:\n");
    ::fprintf(stdout, "  -p <port-to-listen> <class> [<AF_INET/AF_INET6>]    =  Specify a port to listen in [rtmp]\n");
    ::fflush(stdout);
}

class global_data {
    hare::timezone time_zone_ {};
    hare::ptr<hare::log::async> async_ { nullptr };
    hare::ptr<hare::streaming::serve> stream_serve_ { nullptr };

    bool print_stdout_ { true };

public:
    static auto instance() -> global_data&
    {
        static global_data s_info {};
        return s_info;
    }

    ~global_data() = default;

    void init(int argc, char** argv)
    {
        auto log_path = std::string("log/");
        std::list<std::tuple<int16_t, hare::streaming::PROTOCOL_TYPE, int8_t>> listen_ports {};

        for (auto i = 0; i < argc; ++i) {
            if (std::string(argv[i]) == "--help") {
                help();
                ::exit(0);
            }
            if (std::string(argv[i]) == "-p" && i + 2 < argc) {
                auto port = std::stoi(std::string(argv[i + 1]));
                auto type = std::string(argv[i + 2]);
                auto family = AF_INET;
                if (i + 3 < argc) {
                    if (std::string(argv[i + 2]) == "AF_INET6") {
                        family = AF_INET6;
                    }
                }
                listen_ports.emplace_back(port,
                    type == "RTMP" ? hare::streaming::PROTOCOL_TYPE_RTMP : hare::streaming::PROTOCOL_TYPE_INVALID,
                    family);
            }
        }

        if (log_path == "log/") {
            log_path = hare::util::system_dir() + log_path;
            if (::access(log_path.c_str(), 0) != 0) {
                if (::mkdir(log_path.c_str(), DEFAULT_DIR_MODE) == -1) {
                    SYS_FATAL() << "fail to reate dir of log[" << log_path << "].";
                }
            }
            log_path += "stream_serve";
        }

        hare::logger::set_level(hare::log::LEVEL_DEBUG);
        async_ = std::make_shared<hare::log::async>(DEFAULT_ROLL_SIZE, std::move(log_path));
        async_->start();

#ifdef HARE__HAVE_EPOLL
        stream_serve_ = std::make_shared<hare::streaming::serve>(hare::io::cycle::REACTOR_TYPE_EPOLL);
#elif defined(HARE__HAVE_POLL)
        stream_serve_ = std::make_shared<hare::streaming::serve>(hare::io::cycle::REACTOR_TYPE_POLL);
#endif

        if (!stream_serve_) {
            LOG_FATAL() << "cannot create instance of streaming serve.";
        }

        for (const auto& port : listen_ports) {
            auto ret = stream_serve_->listen(std::get<0>(port), std::get<1>(port), std::get<2>(port));
            if (ret) {
                LOG_INFO() << "streaming serve start to listening port[" << std::get<0>(port) << ", type=" << std::get<1>(port) << "].";
            } else {
                LOG_ERROR() << "failed to listen port[" << std::get<0>(port) << ", type=" << std::get<1>(port) << "].";
            }
        }
    }

    void clear()
    {
        if (stream_serve_) {
            stream_serve_->stop();
        }

        if (async_) {
            async_->stop();
            async_.reset();
        }
    }

    inline auto serve() -> hare::ptr<hare::streaming::serve> { return stream_serve_; }

private:
    global_data()
        : time_zone_(EAST_OF_UTC, "Z")
    {
        hare::logger::set_timezone(time_zone_);
        hare::logger::set_output(std::bind(&global_data::output_log, this, std::placeholders::_1, std::placeholders::_2));
    }

    void output_log(const char* log_line, int32_t size)
    {
        if (async_) {
            async_->append(log_line, size);
        }

        if (print_stdout_) {
            ::fprintf(stdout, "%s", log_line);
        }
    }
};

} // namespace detail

auto main(int argc, char** argv) -> int32_t
{
    hare::util::set_tname("HARE_STREAM");

    hare::streaming::serve::start_logo();

    detail::global_data::instance().init(argc, argv);

    detail::global_data::instance().serve()->run();

    detail::global_data::instance().clear();

    return (0);
}