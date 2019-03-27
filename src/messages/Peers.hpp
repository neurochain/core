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
  const KeyPub _own_key;
  mutable std::shared_mutex _mutex;
  using PeersByKey =
      std::unordered_map<KeyPub, std::unique_ptr<Peer>, PacketHash<KeyPub>>;
  PeersByKey _peers;

 public:
  class iterator {
   private:
    using Indexes = std::vector<Peer *>;
    Peer::Status _status;
    Indexes _peers;
    Indexes::iterator _it;

    void shuffle() {
      std::mt19937 g(_rd());
      std::shuffle(_peers.begin(), _peers.end(), g);
      _it = _peers.begin();
    }

   public:
    iterator() = default;

    iterator(const PeersByKey &peers, const Peer::Status status)
        : _status(status) {
      for (const auto &pair : peers) {
        if (pair.second->status() & _status) {
          _peers.push_back(pair.second.get());
        }
      }
      shuffle();
    }

    iterator(const PeersByKey &peers) {
      for (const auto &pair : peers) {
        _peers.push_back(pair.second.get());
      }
      shuffle();
    }

    void operator++() {
      do {
        ++_it;
      } while (_it != _peers.end() && !((*_it)->status() & _status));
    }

    bool operator==(const iterator &) { return _it == _peers.end(); }
    bool operator!=(const iterator &) { return _it != _peers.end(); }
    Peer *operator*() { return *_it; }
    Peer *operator->() { return *_it; }
  };

  Peers(const KeyPub &own_key) : _own_key(own_key) {}

  template <typename It>
  Peers(const KeyPub &own_key, It it, It end) : Peers(own_key) {
    for (; it != end; ++it) {
      insert(*it);
    }
  }

  bool load(const std::string &path) { return false; }
  bool save(const std::string &path) { return false; }
  std::size_t size() const { return _peers.size(); }

  std::optional<Peer *> insert(const Peer &peer);

  bool fill(_Peers *peers, uint8_t peer_count = 10);
  std::size_t used_peers_count() const;
  void update_unreachable();
  bool update_peer_status(const Peer &peer, const Peer::Status status);
  std::optional<Peer *> find(const KeyPub &key_pub);
  std::vector<Peer *> by_status(const Peer::Status status);
  std::vector<Peer *> used_peers();
  std::vector<Peer *> connected_peers();
  std::vector<Peer> peers_copy() const;
  iterator begin();
  iterator begin(const Peer::Status);
  iterator end();
};

std::ostream &operator<<(std::ostream &os, const Peers &peers);

}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_PEERS_HPP */
