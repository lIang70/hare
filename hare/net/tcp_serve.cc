#include "hare/base/util/util.h"
#include "hare/net/core/cycle.h"
#include "hare/net/tcp_serve_p.h"
#include "hare/net/tcp_session_p.h"
#include <hare/base/logging.h>

namespace hare {
namespace net {

    namespace detail {

        void Acceptor::eventCallBack(int32_t events, const Timestamp& receive_time)
        {
            ownerCycle()->assertInCycleThread();
            if ((events & EVENT_READ) != 0) {
                HostAddress peer_addr {};
                // FIXME loop until no more ?
                auto accepted { false };
                util_socket_t conn_fd {};
                while ((conn_fd = socket_.accept(peer_addr)) >= 0) {
                    LOG_TRACE() << "Accepts of " << peer_addr.toIpPort();
                    if (new_session_) {
                        new_session_(conn_fd, peer_addr, receive_time);
                    } else {
                        socket::close(conn_fd);
                    }
                    accepted = true;
                }
                if (!accepted) {
                    SYS_ERROR() << "Cannot accept new connect";
#ifdef H_OS_LINUX
                    // Read the section named "The special problem of
                    // accept()ing when you can't" in libev's doc.
                    // By Marc Lehmann, author of libev.
                    if (errno == EMFILE) {
                        socket::close(idle_fd_);
                        idle_fd_ = socket::accept(socket_.socket(), nullptr);
                        socket::close(idle_fd_);
                        idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
                    }
#endif
                }
            } else {
                SYS_ERROR() << "Unexpected operation of acceptor.";
            }
        }

    } // namespace detail

    void TcpServePrivate::newSession(util_socket_t target_fd, const HostAddress& address, const Timestamp& time)
    {
        HARE_ASSERT(started_ == true, "Serve already stop...");
        cycle_->assertInCycleThread();
        auto* work_cycle = thread_pool_->getNextCycle();
        char buf[HARE_SMALL_FIXED_SIZE * 2];
        snprintf(buf, sizeof buf, "-%s#%lu", listen_address_.toIpPort().c_str(), session_id_++);
        auto session_name = name_ + buf;
        LOG_DEBUG() << "New session[" << session_name
                    << "] in serve[" << name_
                    << "] from " << listen_address_.toIpPort()
                    << " in " << time.toFormattedString();

        auto local_addr = HostAddress::localAddress(target_fd);
        auto* tsp = new TcpSessionPrivate(work_cycle, session_name, family_, target_fd, local_addr, address);
        auto tied_object = acceptor_->tiedObject().lock();
        if (tied_object) {
            auto* tied_serve = static_cast<TcpServe*>(tied_object.get());
            if (tied_serve == nullptr) {
                goto delete_session;
            }
            auto session = tied_serve->createSession(tsp);
            session->connectEstablished();
            work_cycle->runInLoop(std::bind(&TcpServe::newConnect, tied_serve->shared_from_this(), session, time));
            return;
        }
    delete_session:
        delete tsp;
        SYS_ERROR() << "Cannot get TcpServe...";
    }

    TcpServe::TcpServe(const std::string& type, int8_t family, const std::string& name)
        : p_(new TcpServePrivate)
    {
        p_->reactor_type_ = type;
        p_->name_ = name;
        p_->family_ = family;
    }

    TcpServe::~TcpServe()
    {
        HARE_ASSERT(p_ != nullptr && !p_->started_, "");
        delete p_;
    }

    void TcpServe::setReusePort(bool is_reuse)
    {
        HARE_ASSERT(p_->started_ == false, "The serve already running...");
        p_->reuse_port_ = is_reuse;
    }

    void TcpServe::setThreadNum(int32_t num)
    {
        HARE_ASSERT(p_->started_ == false, "The serve already running...");
        p_->thread_num_ = num;
    }

    void TcpServe::listen(const HostAddress& address)
    {
        HARE_ASSERT(p_->started_ == false, "The serve already running...");
        p_->listen_address_ = address;
    }

    auto TcpServe::isRunning() const -> bool
    {
        return p_->started_.load();
    }

    auto TcpServe::addTimer(net::Timer* timer) -> TimerId
    {
        if (!isRunning()) {
            return 0;
        }

        return p_->cycle_->addTimer(timer);
    }

    void TcpServe::cancel(net::TimerId timer_id)
    {
        p_->cycle_->cancel(timer_id);
    }

    void TcpServe::exec()
    {
        p_->cycle_.reset(new core::Cycle(p_->reactor_type_));
        HARE_ASSERT(p_->cycle_ != nullptr, "Cannot create cycle.");

        p_->acceptor_.reset(new detail::Acceptor(p_->cycle_.get(), p_->family_, socket::createNonblockingOrDie(p_->family_), p_->reuse_port_));
        p_->acceptor_->socket_.bindAddress(p_->listen_address_);
        p_->acceptor_->new_session_ = std::bind(&TcpServePrivate::newSession, p_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        p_->acceptor_->listen();
        p_->acceptor_->tie(shared_from_this());

        p_->thread_pool_ = std::make_shared<core::CycleThreadPool>(p_->cycle_.get(), p_->reactor_type_, p_->name_ + "_WORKER");
        p_->thread_pool_->setThreadNum(p_->thread_num_);
        p_->thread_pool_->start();

        p_->started_.exchange(true);
        p_->cycle_->loop();
        p_->started_.exchange(false);

        p_->thread_pool_.reset();
        p_->acceptor_.reset();
        p_->cycle_.reset();
    }

    void TcpServe::exit()
    {
        p_->cycle_->exit();
    }

} // namespace net
} // namespace hare
