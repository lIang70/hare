#include "main/manage/input.h"

#include <hare/base/log/async.h>
#include <hare/base/logging.h>
#include <hare/base/util/system_info.h>
#include <hare/hare-config.h>
#include <hare/net/acceptor.h>
#include <hare/net/host_address.h>

#ifdef HARE__HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HARE__HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef H_OS_WIN32
#else
#include <unistd.h>
#endif

#define DEFAULT_DIR_MODE (0755)
#define DEFAULT_ROLL_SIZE (20 * 1024 * 1024) // 20MB
#define EAST_OF_UTC (8 * 3600) // 8-hours

namespace manage {
namespace detail {

    class global_log {
        hare::timezone time_zone_ {};
        hare::log::async::ptr async_ { nullptr };
        bool print_stdout_ { true };

    public:
        static auto instance() -> hare::ptr<global_log>
        {
            static hare::ptr<global_log> s_glog {};
            return s_glog;
        }

        global_log(int32_t east_of_utc, std::string path_of_log, int32_t roll_size)
            : time_zone_(east_of_utc, "Z")
            , async_(new hare::log::async(roll_size, std::move(path_of_log)))
        {
            hare::logger::set_timezone(time_zone_);
            hare::logger::set_output(std::bind(&global_log::output_log, this, std::placeholders::_1, std::placeholders::_2));
            async_->start();
        }

        ~global_log()
        {
            async_->stop();
            async_.reset();
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

    void help()
    {
        ::fprintf(stdout, "Usage\n");
        ::fprintf(stdout, "\n");
        ::fprintf(stdout, "  hare [options]\n");
        ::fprintf(stdout, "\n");
        ::fprintf(stdout, "Options\n");
        ::fprintf(stdout, "  -p <port-to-listen> <class> [<AF_INET/AF_INET6>]   =  Specify a port to listen in [tcp/udp]\n");
        ::fflush(stdout);
    }
} // namespace detail

auto input::instance() -> input&
{
    static input s_input;
    return s_input;
}

input::~input()
{
    listen_ports_.clear();
    detail::global_log::instance().reset();
}

void input::init(int32_t argc, char** argv)
{
    auto is_help { false };
    auto log_path = std::string("log/");

    for (auto i = 0; i < argc; ++i) {
        if (std::string(argv[i]) == "--help") {
            is_help = true;
            break;
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
            listen_ports_.emplace_back(port, type == "TCP" ? hare::net::TYPE_TCP : hare::net::TYPE_UDP, family);
        }
    }

    if (is_help) {
        detail::help();
        ::exit(0);
    }

    if (log_path == "log/") {
        auto tmp = hare::util::system_dir() + log_path;
        tmp.swap(log_path);
        if (::access(log_path.c_str(), 0) != 0) {
            if (::mkdir(log_path.c_str(), DEFAULT_DIR_MODE) == -1) {
                SYS_FATAL() << "fail to reate dir of log[" << log_path << "].";
            }
        }
        log_path += "stream_serve";
    }

    detail::global_log::instance().reset(new detail::global_log(EAST_OF_UTC, log_path, DEFAULT_ROLL_SIZE));
}

auto input::init_serve(hare::core::stream_serve& serve_ptr) -> bool
{
    for (const auto& port : listen_ports_) {
        serve_ptr.listen(std::get<0>(port), std::get<1>(port), std::get<2>(port));
    }

    hare::logger::set_level(hare::log::LEVEL_DEBUG);

    return true;
}

input::input() = default;

} // namespace manage
