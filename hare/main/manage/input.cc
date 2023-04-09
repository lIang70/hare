#include "main/manage/input.h"

#include <hare/base/log/async.h>
#include <hare/base/logging.h>
#include <hare/base/time/time_zone.h>
#include <hare/base/util/system_info.h>
#include <hare/hare-config.h>
#include <hare/net/acceptor.h>
#include <hare/net/host_address.h>

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>

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

namespace hare {
namespace manage {

    namespace detail {

        class GlobalLog {
            TimeZone time_zone_ {};
            log::Async::Ptr async_ { nullptr };
            bool print_stdout_ { true };

        public:
            GlobalLog(int32_t east_of_utc, std::string path_of_log, int32_t roll_size)
                : time_zone_(east_of_utc, "Z")
                , async_(new log::Async(std::move(path_of_log), roll_size))
            {
                hare::Logger::setTimeZone(time_zone_);
                hare::Logger::setOutput(std::bind(&GlobalLog::outputLog, this, std::placeholders::_1, std::placeholders::_2));
                async_->start();
            }

            ~GlobalLog()
            {
                async_->stop();
                async_.reset();
            }

            void outputLog(const char* log_line, int32_t size)
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
            ::fprintf(stdout, "  -P <port-to-listen> [<class>]   =  Specify a port to listen in [AF_INET/AF_INET6]\n");
            ::fflush(stdout);
        }
    } // namespace detail

    std::unique_ptr<detail::GlobalLog> global_log {};

    auto Input::instance() -> Input&
    {
        static Input s_input;
        return s_input;
    }

    Input::~Input()
    {
        input_thread_.join();
        listen_ports_.clear();
        global_log.reset();
    }

    void Input::init(int32_t argc, char** argv)
    {
        auto is_help { false };
        auto log_path = std::string("log/");

        for (auto i = 0; i < argc; ++i) {
            if (std::string(argv[i]) == "--help") {
                is_help = true;
                break;
            }
            if (std::string(argv[i]) == "-P" && i + 1 < argc) {
                auto port = std::stoi(std::string(argv[i + 1]));
                auto family = AF_INET;
                if (i + 2 < argc) {
                    if (std::string(argv[i + 2]) == "AF_INET6") {
                        family = AF_INET6;
                    }
                }
                listen_ports_.emplace_back(port, family);
            }
        }

        if (is_help) {
            detail::help();
            ::exit(0);
        }

        if (log_path == "log/") {
            auto tmp = util::systemDir() + log_path;
            tmp.swap(log_path);
            if (::access(log_path.c_str(), 0) != 0) {
                if (::mkdir(log_path.c_str(), DEFAULT_DIR_MODE) == -1) {
                    LOG_FATAL() << "Fail to reate dir of log[" << log_path << "].";
                }
            }
            log_path += "stream_serve";
        }

        global_log.reset(new detail::GlobalLog(EAST_OF_UTC, log_path, DEFAULT_ROLL_SIZE));
    }

    auto Input::initServe(core::StreamServe::Ptr& serve_ptr) -> bool
    {
        for (const auto& port : listen_ports_) {
            net::Acceptor::Ptr acceptor = std::make_shared<net::Acceptor>(
                std::get<1>(port), static_cast<int16_t>(std::get<0>(port)), true);
            serve_ptr->add(acceptor);
        }

        serve_ptr->setThreadNum(static_cast<int32_t>(std::thread::hardware_concurrency()));

        Logger::setLevel(log::Level::LOG_DEBUG);

        return true;
    }

    Input::Input()
        : input_thread_(std::bind(&Input::run, this), "INPUT")
    {
        // input_thread_.start();
    }

    void Input::run()
    {
    }

} // namespace manage
} // namespace hare
