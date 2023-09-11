/**
 * @file hare/net/tcp/client.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with client.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_TCP_CLIENT_H_
#define _HARE_NET_TCP_CLIENT_H_

#include <hare/net/host_address.h>
#include <hare/net/tcp/session.h>

namespace hare {
namespace net {

static const std::int64_t kRetryDelay =
    1L * HARE_MICROSECONDS_PER_SECOND;  // 1s

HARE_CLASS_API
class HARE_API TcpClient : public util::NonCopyable {
  hare::detail::Impl* impl_{};

 public:
  explicit TcpClient(io::Cycle* _cycle, std::string _name = "HARE_CLIENT");
  virtual ~TcpClient();

  auto MainCycle() const -> io::Cycle*;
  auto IsRunning() const -> bool;
  void SetConnectCallback(TcpSession::ConnectCallback _connect);

  auto Connected() -> bool;
  void ConnectTo(const HostAddress& _address, bool _retry = true,
                 std::int32_t _retry_times = 30,
                 std::int64_t _delay = kRetryDelay);
  void Close();

  auto Send(const void* _buffer, std::size_t _len) -> bool;
  auto Append(Buffer& _buffer) -> bool;

 private:
  void DestroySession();
};

}  // namespace net
}  // namespace hare

#endif  // _HARE_NET_TCP_CLIENT_H_