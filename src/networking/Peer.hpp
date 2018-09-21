#ifndef NEURO_SRC_NETWORKING_PEER_HPP
#define NEURO_SRC_NETWORKING_PEER_HPP

#include <boost/asio/ip/address.hpp>
#include <optional>
#include <string>

#include "common/types.hpp"
#include "crypto/EccPub.hpp"
#include "common/logger.hpp"

namespace neuro {
namespace networking {

class Peer {
public:
  enum class Status { CONNECTED, CONNECTING, REACHABLE, UNREACHABLE, FULL };

private:
  const IP _ip;
  const Port _port;
  const std::optional<const crypto::EccPub> _key_pub;
  Status _status{Status::REACHABLE};

public:
  Peer(const std::string ip, const Port port,
       std::optional<const crypto::EccPub> key_pub = {})
      : _ip(boost::asio::ip::make_address_v4(ip)), _port(port),
        _key_pub(key_pub) {}
  Peer(const IP ip, const Port port,
       std::optional<const crypto::EccPub> key_pub = {})
      : _ip(ip), _port(port), _key_pub(key_pub) {}
  Peer(const Peer &peer) = default;
  Peer(Peer &&peer) = default;
  Peer &operator=(const Peer &peer) = default;
  const IP ip() const { return _ip; }
  const Port port() const { return _port; }
  const std::optional<const crypto::EccPub> key_pub() const { return _key_pub; }
  Status status() const { return _status; }
  void status(Status status) { _status = status; }
  ~Peer() {}
};

using Peers = std::vector<std::shared_ptr<Peer>>;
std::ostream &operator<<(std::ostream &os, const Peer &peer);
std::ostream &operator<<(std::ostream &os, const Peers &peers);
bool operator==(const Peer &a, const Peer &b);
bool operator==(const Peers &a, const Peers &b);

} // namespace networking
} // namespace neuro

#endif /* NEURO_SRC_NETWORKING_PEER_HPP */
