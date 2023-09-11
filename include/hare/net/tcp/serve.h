/**
 * @file hare/net/hybrid_serve.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with hybrid_serve.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_HYBRID_SERVE_H_
#define _HARE_NET_HYBRID_SERVE_H_

#include <hare/net/tcp/session.h>

namespace hare {
namespace net {

class Acceptor;

HARE_CLASS_API
class HARE_API TcpServe : public util::NonCopyable {
  hare::detail::Impl* impl_{};

 public:
  using NewSessionCallback = std::function<void(
      const Ptr<TcpSession>&, const Timestamp&, const Ptr<Acceptor>&)>;

  explicit TcpServe(io::Cycle* _cycle, std::string _name = "HARE_SERVE");
  virtual ~TcpServe();

  auto MainCycle() const -> io::Cycle*;
  auto IsRunning() const -> bool;
  void SetNewSession(NewSessionCallback _new_session);

  auto AddAcceptor(const Ptr<Acceptor>& _acceptor) -> bool;

  auto Exec(std::int32_t _thread_nbr) -> Error;
  void Exit();

 private:
  void NewSession(util_socket_t _fd, HostAddress& _address,
                  const Timestamp& _time, Acceptor* _acceptor);
};

}  // namespace net
}  // namespace hare

#endif  // _HARE_NET_TCP_SERVE_H_