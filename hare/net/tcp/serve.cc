#include "hare/base/fwd-inl.h"
#include "hare/base/io/socket_op-inl.h"
#include "hare/net/io_pool.h"
#include <hare/base/io/cycle.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/hare-config.h>
#include <hare/net/tcp/acceptor.h>
#include <hare/net/tcp/serve.h>

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if defined(H_OS_WIN)
#include <WinSock2.h>
#include <Ws2tcpip.h>
#endif

namespace hare {
namespace net {

    HARE_IMPL_DEFAULT(
        TcpServe,
        std::string name_ {};

        // the acceptor loop
        io::Cycle * cycle_ {};
        Ptr<IOPool<Ptr<TcpSession>>> io_pool_ {};
        std::uint64_t session_id_ { 0 };
        bool started_ { false };

        TcpServe::NewSessionCallback new_session_ {};
    )

    TcpServe::TcpServe(io::Cycle* _cycle, std::string _name)
        : impl_(new TcpServeImpl)
    {
        d_ptr(impl_)->name_ = std::move(_name);
        d_ptr(impl_)->cycle_ = _cycle;
    }

    TcpServe::~TcpServe()
    {
        assert(!d_ptr(impl_)->started_);
        delete impl_;
    }

    auto TcpServe::MainCycle() const -> io::Cycle*
    {
        return d_ptr(impl_)->cycle_;
    }

    auto TcpServe::IsRunning() const -> bool
    {
        return d_ptr(impl_)->started_;
    }

    void TcpServe::SetNewSession(NewSessionCallback _new_session)
    {
        d_ptr(impl_)->new_session_ = std::move(_new_session);
    }

    auto TcpServe::AddAcceptor(const Ptr<Acceptor>& _acceptor) -> bool
    {
        util::CountDownLatch cdl(1);
        auto in_cycle = d_ptr(impl_)->cycle_->InCycleThread();
        auto added { false };

        d_ptr(impl_)->cycle_->RunInCycle([&] {
            d_ptr(impl_)->cycle_->EventUpdate(_acceptor);
            _acceptor->SetNewSession(std::bind(&TcpServe::NewSession, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            auto ret = _acceptor->Listen();
            if (!ret) {
                MSG_ERROR("acceptor[{}] cannot listen.", _acceptor->Socket());
                cdl.CountDown();
                return;
            }

            MSG_TRACE("add acceptor[{}], port={}, type={}] to serve[{}].",
                _acceptor->Socket(), _acceptor->Port(), TypeToStr(TYPE_TCP), (void*)this);
            added = true;
            cdl.CountDown();
        });

        if (!in_cycle) {
            cdl.Await();
        }

        return added;
    }

    auto TcpServe::Exec(std::int32_t _thread_nbr) -> Error
    {
        assert(d_ptr(impl_)->cycle_ != nullptr);

        d_ptr(impl_)->io_pool_ = std::make_shared<IOPool<Ptr<TcpSession>>>("SERVER_WORKER");
        auto ret = d_ptr(impl_)->io_pool_->Start(d_ptr(impl_)->cycle_->type(), _thread_nbr);
        if (!ret) {
            return Error(ERROR_INIT_IO_POOL);
        }

        d_ptr(impl_)->started_ = true;
        d_ptr(impl_)->cycle_->Exec();
        d_ptr(impl_)->started_ = false;

        MSG_TRACE("clean io pool...");
        d_ptr(impl_)->io_pool_->Stop();
        d_ptr(impl_)->io_pool_.reset();

        return Error();
    }

    void TcpServe::Exit()
    {
        d_ptr(impl_)->cycle_->Exit();
    }

    void TcpServe::NewSession(util_socket_t _fd, HostAddress& _address, const Timestamp& _time, Acceptor* _acceptor)
    {
        assert(d_ptr(impl_)->started_);
        d_ptr(impl_)->cycle_->AssertInCycleThread();

        auto next_item = d_ptr(impl_)->io_pool_->GetNextItem();
        Ptr<TcpSession> tcp_session { nullptr };
        std::string name_cache {};

        assert(_fd != -1);
        auto local_addr = HostAddress::LocalAddress(_fd);

        name_cache = fmt::format("{}-{}#tcp{}", d_ptr(impl_)->name_, local_addr.ToIpPort(), d_ptr(impl_)->session_id_++);

        MSG_TRACE("new session[{}] in serve[{}] from {} in {}.",
            name_cache, d_ptr(impl_)->name_, _address.ToIpPort(), _time.ToFmt());

        tcp_session.reset(new TcpSession(next_item->cycle.get(),
            std::move(local_addr),
            name_cache, _acceptor->Family(), _fd,
            std::move(_address)));

        if (!tcp_session) {
            MSG_ERROR("fail to create session[{}].", name_cache);
            socket_op::Close(_fd);
            return;
        }

        auto sfd = tcp_session->Fd();

        tcp_session->SetDestroy([=]() {
            next_item->cycle->RunInCycle([=]() mutable {
                assert(next_item->sessions.find(sfd) != next_item->sessions.end());
                next_item->sessions.erase(sfd);
            });
        });

        next_item->cycle->RunInCycle([=]() mutable {
            assert(next_item->sessions.find(sfd) == next_item->sessions.end());
            if (!d_ptr(impl_)->new_session_) {
                MSG_ERROR("you need register new_session_callback to serve[{}].", d_ptr(impl_)->name_);
                if (sfd != -1) {
                    socket_op::Close(sfd);
                }
                return;
            }

            next_item->sessions.insert(std::make_pair(sfd, tcp_session));
            d_ptr(impl_)->new_session_(tcp_session, _time, std::static_pointer_cast<Acceptor>(_acceptor->shared_from_this()));
            tcp_session->ConnectEstablished();
        });
    }

} // namespace net
} // namespace hare