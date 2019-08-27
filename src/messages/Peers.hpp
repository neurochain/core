#ifndef NEURO_SRC_MESSAGES_PEERS_HPP
#define NEURO_SRC_MESSAGES_PEERS_HPP

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <shared_mutex>
#include <string>
#include <vector>

#include "common.pb.h"
#include "common/logger.hpp"
#include "config.pb.h"
#include "messages.pb.h"
#include "messages/Message.hpp"
#include "messages/Peer.hpp"

namespace neuro {
namespace messages {

class Peers {
 private:
  const _KeyPub _own_key;
  mutable std::shared_mutex _mutex;
  using PeersByKey =
      std::unordered_map<_KeyPub, std::unique_ptr<Peer>, PacketHash<_KeyPub>>;
  PeersByKey _peers;

 public:
  class iterator {
   private:
    using Indexes = std::vector<Peer *>;
    Peer::Status _status;
    Indexes _peers;
    Indexes::iterator _it;
    static constexpr auto ALLSTATUS =
        static_cast<Peer::Status>(Peer::CONNECTED | Peer::CONNECTED |
                                  Peer::UNREACHABLE | Peer::DISCONNECTED);

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

    iterator(const PeersByKey &peers) : _status(ALLSTATUS) {
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

  Peers(const _KeyPub &own_key, const messages::config::Networking &config)
      : _own_key(own_key) {
    for (auto configured_peer : config.tcp().peers()) {
      messages::Peer peer(config, configured_peer);
      insert(peer);
    }
  }

  bool load(const std::string &path) { return false; }
  bool save(const std::string &path) { return false; }
  std::size_t size() const { return _peers.size(); }

  std::optional<Peer *> insert(const Peer &peer);

  void fill(_Peers *peers, uint8_t peer_count = 10);
  std::size_t used_peers_count() const;
  void update_unreachable();
  std::optional<Peer *> find(const _KeyPub &key_pub);
  std::vector<Peer *> by_status(const Peer::Status status);
  std::vector<Peer *> used_peers();
  std::vector<Peer *> connected_peers();
  std::vector<Peer> peers_copy() const;
  std::optional<Peer *> peer_by_port(const Port port) const;
  iterator begin();
  const iterator begin() const;
  iterator begin(const Peer::Status);
  iterator end();
  const iterator end() const;
  operator _Peers() const;
};

std::ostream &operator<<(std::ostream &os, const Peers &peers);

}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_PEERS_HPP */
