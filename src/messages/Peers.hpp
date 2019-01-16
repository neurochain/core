#ifndef NEURO_SRC_MESSAGES_PEERS_HPP
#define NEURO_SRC_MESSAGES_PEERS_HPP

#include <algorithm>
#include <numeric>
#include <optional>
#include <string>
#include <vector>
#include "common.pb.h"
#include "messages/Message.hpp"

namespace neuro {
namespace messages {

class Peers : public _Peers {
 private:
  int _used_status;

 public:
  Peers() { _used_status = (Peer::CONNECTING | Peer::CONNECTED); }
  Peers(const std::string &path) { from_json_file(path, this); }

  template <typename It>
  Peers(It it, It end) {
    for (; it != end; ++it) {
      insert(*it);
    }
  }
  
  bool load(const std::string &path) { return false; }
  bool save(const std::string &path) { return false; }
  std::size_t size() const { return peers().size(); }

  Peer *insert(const Peer &peer) {
    auto it = std::find(mutable_peers()->begin(), mutable_peers()->end(), peer);
    if (it != mutable_peers()->end()) {
      return &(*it);
    }

    auto new_peer = add_peers();
    new_peer->set_status(messages::Peer::DISCONNECTED);
    new_peer->CopyFrom(peer);

    return new_peer;
  }

  Peer *insert(const messages::KeyPub &key_pub,
               const std::optional<Endpoint> &endpoint,
               const std::optional<Port> &listen_port) {
    auto it = std::find_if(
        mutable_peers()->begin(), mutable_peers()->end(),
        [&key_pub](const Peer &peer) { return (peer.key_pub() == key_pub); });
    if (it != mutable_peers()->end()) {
      return &(*it);
    }

    auto new_peer = add_peers();
    new_peer->set_status(messages::Peer::DISCONNECTED);
    if (endpoint) {
      new_peer->set_endpoint(*endpoint);
    }
    if (listen_port) {
      new_peer->set_port(*listen_port);
    }
    set_last_update(new_peer);
    return new_peer;
  }

  void set_last_update(Peer *peer) {
    peer->mutable_last_update()->set_data(std::time(nullptr));
  }

  std::size_t used_peers_count() const {
    return std::count_if(
        peers().begin(), peers().end(),
        [this](const Peer &peer) { return (_used_status | peer.status()); });
  }

  bool update_peer_status(const Peer &peer, const Peer::Status status) {
    auto got =
        std::find(mutable_peers()->begin(), mutable_peers()->end(), peer);
    if (got == mutable_peers()->end()) {
      return false;
    }
    got->set_status(status);
    return true;
  }

  Peer *find_random(const Peer::Status status, const Duration &duration) {
    std::vector<std::size_t> indexes(peers().size());
    std::iota(indexes.begin(), indexes.end(), 0);
    std::random_shuffle(indexes.begin(), indexes.end());
    Peer *peer{nullptr};
    for (const auto index : indexes) {
      if (peer = mutable_peers(index); (peer->status() == Peer::CONNECTING ||
                                        peer->status() == Peer::CONNECTED)) {
        break;
      }
    }

    return peer;
  }

  std::vector<Peer> peers_by_status(const Peer::Status status) const {
    std::vector<Peer> res;
    std::copy_if(
        peers().begin(), peers().end(), std::back_inserter(res),
        [status](const Peer &peer) { return (peer.status() | status); });

    return res;
  }

  std::vector<Peer> used_peers() const {
    return peers_by_status(Peer::Status(_used_status));
  }

  std::vector<Peer> connected_peers() const {
    return peers_by_status(Peer::CONNECTED);
  }

  ~Peers() {
    std::cout << "Killing Peers" << std::endl;
  }
};

}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_PEERS_HPP */
