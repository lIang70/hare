#include "base/fwd-inl.h"
#include "net/io_pool.h"
#include "socket_op.h"
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
        std::string name {};

        // the acceptor loop
        io::Cycle * cycle {};
        Ptr<IOPool<Ptr<TcpSession>>> io_pool {};
        std::uint64_t session_id { 0 };
        bool started { false };

        TcpServe::NewSessionCallback new_session {};
    )

    TcpServe::TcpServe(io::Cycle* _cycle, std::string _name)
        : impl_(new TcpServeImpl)
    {
        IMPL->name = std::move(_name);
        IMPL->cycle = _cycle;
    }

    TcpServe::~TcpServe()
    {
        HARE_ASSERT(!IMPL->started);
        delete impl_;
    }

    auto TcpServe::MainCycle() const -> io::Cycle*
    {
        return IMPL->cycle;
    }

    auto TcpServe::IsRunning() const -> bool
    {
        return IMPL->started;
    }

    void TcpServe::SetNewSession(NewSessionCallback _new_session)
    {
        IMPL->new_session = std::move(_new_session);
    }

    auto TcpServe::AddAcceptor(const Ptr<Acceptor>& _acceptor) -> bool
    {
        util::CountDownLatch cdl(1);
        auto in_cycle = IMPL->cycle->InCycleThread();
        auto added { false };

        IMPL->cycle->RunInCycle([&] {
            IMPL->cycle->EventUpdate(_acceptor);
            _acceptor->SetNewSession(std::bind(&TcpServe::NewSession, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            auto ret = _acceptor->Listen();
            if (!ret) {
                HARE_INTERNAL_ERROR("acceptor[{}] cannot listen.", _acceptor->Socket());
                cdl.CountDown();
                return;
            }

            HARE_INTERNAL_TRACE("add acceptor[{}], port={}, type={}] to serve[{}].",
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
        HARE_ASSERT(IMPL->cycle != nullptr);

        IMPL->io_pool = std::make_shared<IOPool<Ptr<TcpSession>>>("SERVER_WORKER");
        auto ret = IMPL->io_pool->Start(IMPL->cycle->type(), _thread_nbr);
        if (!ret) {
            return Error(ERROR_INIT_IO_POOL);
        }

        IMPL->started = true;
        IMPL->cycle->Exec();
        IMPL->started = false;

        HARE_INTERNAL_TRACE("clean io pool...");
        IMPL->io_pool->Stop();
        IMPL->io_pool.reset();

        return Error();
    }

    void TcpServe::Exit()
    {
        IMPL->cycle->Exit();
    }

    void TcpServe::NewSession(util_socket_t _fd, HostAddress& _address, const Timestamp& _time, Acceptor* _acceptor)
    {
        HARE_ASSERT(IMPL->started);
        IMPL->cycle->AssertInCycleThread();

        auto next_item = IMPL->io_pool->GetNextItem();
        Ptr<TcpSession> tcp_session { nullptr };
        std::string name_cache {};

        HARE_ASSERT(_fd != -1);
        auto local_addr = HostAddress::LocalAddress(_fd);

        name_cache = fmt::format("{}-{}#tcp{}", IMPL->name, local_addr.ToIpPort(), IMPL->session_id++);

        HARE_INTERNAL_TRACE("new session[{}] in serve[{}] from {} in {}.",
            name_cache, IMPL->name, _address.ToIpPort(), _time.ToFmt());

        tcp_session.reset(new TcpSession(next_item->cycle.get(),
            std::move(local_addr),
            name_cache, _acceptor->Family(), _fd,
            std::move(_address)));

        if (!tcp_session) {
            HARE_INTERNAL_ERROR("fail to create session[{}].", name_cache);
            socket_op::Close(_fd);
            return;
        }

        auto sfd = tcp_session->Fd();

        tcp_session->SetDestroy([=]() {
            next_item->cycle->RunInCycle([=]() mutable {
                HARE_ASSERT(next_item->sessions.find(sfd) != next_item->sessions.end());
                next_item->sessions.erase(sfd);
            });
        });

        next_item->cycle->RunInCycle([=]() mutable {
            HARE_ASSERT(next_item->sessions.find(sfd) == next_item->sessions.end());
            if (!IMPL->new_session) {
                HARE_INTERNAL_ERROR("you need register new_session_callback to serve[{}].", IMPL->name);
                if (sfd != -1) {
                    socket_op::Close(sfd);
                }
                return;
            }

            next_item->sessions.insert(std::make_pair(sfd, tcp_session));
            IMPL->new_session(tcp_session, _time, std::static_pointer_cast<Acceptor>(_acceptor->shared_from_this()));
            tcp_session->ConnectEstablished();
        });
    }

} // namespace net
} // namespace hare