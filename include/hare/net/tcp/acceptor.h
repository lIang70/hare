/**
 * @file hare/net/acceptor.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with acceptor.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_ACCEPTOR_H_
#define _HARE_NET_ACCEPTOR_H_

#include <hare/base/io/event.h>
#include <hare/base/time/timestamp.h>
#include <hare/net/host_address.h>
#include <hare/net/socket.h>

namespace hare {
namespace net {

HARE_CLASS_API
class HARE_API Acceptor : public io::Event {
  hare::detail::Impl* impl_{};

#ifdef H_OS_LINUX
  // Read the section named "The special problem of
  // accept()ing when you can't" in libev's doc.
  // By Marc Lehmann, author of libev.
  util_socket_t idle_fd_{-1};
#endif

 public:
  using NewSessionCallback = std::function<void(util_socket_t, HostAddress&,
                                                const Timestamp&, Acceptor*)>;

  Acceptor(std::uint8_t _family, std::uint16_t _port, bool _reuse_port = true);
  ~Acceptor() override;

  auto Socket() const -> util_socket_t;
  auto Port() const -> std::uint16_t;
  auto Family() const -> std::uint8_t;

 protected:
  void EventCallback(const Ptr<io::Event>& _event, std::uint8_t _events,
                     const Timestamp& _receive_time);

 private:
  auto Listen() -> Error;

  void SetNewSession(NewSessionCallback _cb);

  friend class TcpServe;
};

}  // namespace net
}  // namespace hare

#endif  // _HARE_NET_ACCEPTOR_H_