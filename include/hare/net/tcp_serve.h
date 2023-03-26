#ifndef _HARE_NET_TCP_SERVE_H_
#define _HARE_NET_TCP_SERVE_H_

#include <hare/net/tcp_session.h>
#include <hare/net/acceptor.h>
#include <hare/net/timer.h>

namespace hare {
namespace net {

    class TcpServePrivate;
    class HARE_API TcpServe : public NonCopyable
                            , public std::enable_shared_from_this<TcpServe> {
        friend class TcpServePrivate;
        TcpServePrivate* p_ { nullptr };

    public:
        using Ptr = std::shared_ptr<TcpServe>; 

        //! @brief Construct a new Tcp Serve object
        //! @param type The type of reactor. EPOLL/POLL
        explicit TcpServe(const std::string& type, const std::string& name = "HARE_SERVE");
        virtual ~TcpServe();

        // Set before exec()
        void setThreadNum(int32_t num);

        auto isRunning() const -> bool;

        // not thread-safe
        auto add(const Acceptor::Ptr& acceptor) -> bool;

        auto add(net::Timer* timer) -> TimerId;
        void cancel(net::TimerId timer_id);

        void exec();
        void exit();

    protected:
        virtual inline auto createSession(TcpSessionPrivate* tsp) -> TcpSession::Ptr
        {
            return TcpSession::Ptr(new TcpSession(tsp));
        }

        virtual void newConnect(TcpSession::Ptr session, Timestamp time) = 0;
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SERVE_H_