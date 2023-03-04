#include "hare/net/core/cycle.h"
#include "hare/net/socket_op.h"
#include "hare/net/tcp_serve_p.h"
#include <hare/base/logging.h>

namespace hare {
namespace net {

    namespace detail {

        void Acceptor::eventCallBack(int32_t events, Timestamp& receive_time)
        {
            ownerCycle()->assertInCycleThread();
            if (events & EV_READ) {
                HostAddress peer_addr {};
                // FIXME loop until no more
                auto conn_fd = socket_.accept(peer_addr);
                if (conn_fd >= 0) {
                    auto host_port = peer_addr.toIpPort();
                    LOG_TRACE() << "Accepts of " << host_port;
//                    if (newConnectionCallback_) {
//                        newConnectionCallback_(conn_fd, peer_addr);
//                    } else {
//                        socket::close(conn_fd);
//                    }
                } else {
                    SYS_ERROR() << "Cannot accept new connect";
                    // Read the section named "The special problem of
                    // accept()ing when you can't" in libev's doc.
                    // By Marc Lehmann, author of libev.
                    if (errno == EMFILE) {
                        socket::close(idle_fd_);
                        idle_fd_ = socket::accept(socket_.socket(), nullptr);
                        socket::close(idle_fd_);
                        idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
                    }
                }
            } else {
                LOG_ERROR() << "Unexpected operation of acceptor.";
            }
        }

    } // namespace detail

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

    void TcpServe::setReusePort(bool b)
    {
        p_->reuse_port_ = b;
    }

    void TcpServe::setThreadNum(int32_t num)
    {
        p_->thread_num_ = num;
    }

    void TcpServe::listen(const HostAddress& address)
    {
        p_->listen_address_ = address;
    }

    void TcpServe::exec()
    {
        p_->cycle_ = new core::Cycle(p_->reactor_type_);
        HARE_ASSERT(p_->cycle_ != nullptr, "Cannot create cycle.");

        p_->acceptor_.reset(new detail::Acceptor(p_->cycle_, socket::createNonblockingOrDie(p_->family_), p_->reuse_port_));
        p_->acceptor_->socket_.bindAddress(p_->listen_address_);
        p_->acceptor_->listen();

        p_->thread_pool_ = std::make_shared<core::CycleThreadPool>(p_->cycle_, p_->reactor_type_, p_->name_);
        p_->thread_pool_->setThreadNum(p_->thread_num_);
        p_->thread_pool_->start();

        p_->cycle_->loop();

        p_->thread_pool_.reset();
        p_->acceptor_.reset();
        delete p_->cycle_;
    }

    void TcpServe::exit()
    {
        p_->cycle_->exit();
    }

} // namespace net
} // namespace hare
