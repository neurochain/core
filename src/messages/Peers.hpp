#ifndef NEURO_SRC_MESSAGES_PEERS_HPP
#define NEURO_SRC_MESSAGES_PEERS_HPP

#include <algorithm>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>
#include "common.pb.h"
#include "common/logger.hpp"
#include "messages/Message.hpp"
#include "messages/Peer.hpp"

namespace neuro {
namespace messages {

class Peers {
 private:
  const int _used_status{(Peer::CONNECTING | Peer::CONNECTED)};
  const KeyPub &_own_key;
  mutable std::shared_mutex _mutex;

  std::unordered_map<KeyPub, std::unique_ptr<Peer>,
		     PacketHash<KeyPub>> _peers;

 public:
  Peers(const KeyPub &own_key) : _own_key(own_key) {}
  // Peers(config::Peers *peers) {
  //   _Peers tmp;
  //   from_json_file(path, &tmp);
  //   for (const auto &peer : tmp.peers()) {
  //     insert(peer);
  //   }
  // }

  template <typename It>
  Peers(const KeyPub &own_key, It it, It end): Peers(own_key) {
    for (; it != end; ++it) {
      insert(*it);
    }
  }

  bool load(const std::string &path) { return false; }
  bool save(const std::string &path) { return false; }
  std::size_t size() const { return _peers.size(); }

  std::optional<Peer *>insert(const Peer &peer);
  // Peer *insert(const messages::KeyPub &key_pub,
  //              const std::optional<Endpoint> &endpoint,
  //              const std::optional<Port> &listen_port);

  bool fill(_Peers *peers);
  std::size_t used_peers_count() const;
  void update_unreachable();
  bool update_peer_status(const Peer &peer, const Peer::Status status);
  std::optional<Peer *> next(const Peer::Status status);
  std::optional<Peer *> find(const KeyPub &key_pub);
  std::vector<Peer *> by_status(const Peer::Status status) const;
  std::vector<Peer *> used_peers() const;
  std::vector<Peer *> connected_peers() const;
  std::vector<Peer> peers_copy() const;
};

std::ostream &operator<<(std::ostream &os, const Peers &peers);

}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_PEERS_HPP */
