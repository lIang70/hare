#ifndef _HARE_NET_TCP_SERVE_H_
#define _HARE_NET_TCP_SERVE_H_

#include <hare/net/tcp_session.h>
#include <hare/net/host_address.h>

namespace hare {
namespace net {

    class TcpServePrivate;
    class HARE_API TcpServe : public NonCopyable
                            , public std::enable_shared_from_this<TcpServe> {
        TcpServePrivate* p_ { nullptr };

    public:
        //! 
        //! @brief Construct a new Tcp Serve object
        //! 
        //! @param type The type of reactor. EPOLL/POLL
        TcpServe(const std::string& type, int8_t family, const std::string& name = "HARE_SERVE");
        virtual ~TcpServe();

        void setReusePort(bool b);
        void setThreadNum(int32_t num);
        void listen(const HostAddress& address);

        void exec();
        void exit();

    protected:
        virtual inline STcpSession createSession(TcpSessionPrivate* p)
        {
            return STcpSession(new TcpSession(p));
        }

        virtual void newConnect(STcpSession& session) = 0;
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SERVE_H_