#include "messages/Peers.hpp"

namespace neuro {
namespace messages {

Peers::iterator Peers::begin(const Peer::Status status) {
  return iterator{_peers, status};
}

Peers::iterator Peers::begin() { return iterator{_peers}; }
const Peers::iterator Peers::begin() const { return iterator{_peers}; }

Peers::iterator Peers::end() { return iterator{}; }
const Peers::iterator Peers::end() const { return iterator{}; }

std::optional<Peer *> Peers::insert(std::shared_ptr<Peer> peer) {
  if (!peer->has_key_pub()) {
    return {};
  }

  if (peer->key_pub() == _own_key) {
    return {};
  }

  std::unique_lock<std::mutex> lock(_mutex);
  const auto previous_peer = _peers[peer->key_pub()];
  _peers[peer->key_pub()] = peer;

  peer->set_status(Peer::DISCONNECTED);
  return peer.get();
}

std::optional<Peer *> Peers::insert(const Peer &peer) {
  return insert(std::make_shared<Peer>(peer));
}

std::optional<Peer *> Peers::upsert(std::shared_ptr<Peer> peer) {
  if (!peer->has_key_pub()) {
    return {};
  }

  if (peer->key_pub() == _own_key) {
    return {};
  }

  std::unique_lock<std::mutex> lock(_mutex);
  auto [found_element, is_inserted] = _peers.emplace(
      std::piecewise_construct, std::forward_as_tuple(peer->key_pub()),
      std::forward_as_tuple(peer));

  auto &found_peer = found_element->second;
  if (!is_inserted) {
    if (found_peer->status() == messages::Peer::DISCONNECTED) {
      // pub key already known, update peer
      found_peer->set_port(peer->port());
      found_peer->set_endpoint(peer->endpoint());
    }
    return found_peer.get();
  }

  found_peer->set_status(Peer::DISCONNECTED);
  return found_peer.get();
}

std::optional<Peer *> Peers::upsert(const Peer &peer) {
  return upsert(std::make_shared<Peer>(peer));
}

/**
 * count the number of bot communicating with us (connecting or connected peers)
 * \return number of connection | connected peers
 */
std::size_t Peers::used_peers_count() const {
  std::unique_lock<std::mutex> lock(_mutex);
  long used_peers_count =
      std::count_if(_peers.begin(), _peers.end(), [](const auto &it) {
        const auto &peer = it.second;
        auto connection_status =
            peer->status() & (Peer::CONNECTED | Peer::CONNECTING);
        return peer->has_status() && connection_status;
      });
  return used_peers_count;
}

std::shared_ptr<Peer> Peers::find(const _KeyPub &key_pub) {
  std::unique_lock<std::mutex> lock(_mutex);
  auto got = _peers.find(key_pub);

  if (got == _peers.end()) {
    return nullptr;
  }

  return got->second;
}

/**
 * Get a list of peer filtered by status
 * The peer obtained this way can still change (have their status changed)
 * \attention apply update_unreachable on each peer as a side effect
 * \param status a status to filter the list
 * \return the filtered list of peer
 */
std::vector<Peer *> Peers::by_status(const Peer::Status status) {
  std::unique_lock<std::mutex> lock(_mutex);
  std::vector<Peer *> res;

  const auto time = ::neuro::time();

  for (const auto &[_, peer] : _peers) {
    peer->update_unreachable(time);
    if (peer->status() & status) {
      res.push_back(peer.get());
    }
  }

  return res;
}

std::vector<Peer *> Peers::used_peers() {
  return by_status(
      static_cast<Peer::Status>(Peer::CONNECTING | Peer::CONNECTED));
}

std::vector<Peer *> Peers::connected_peers() {
  return by_status(Peer::CONNECTED);
}

std::vector<Peer> Peers::peers_copy() const {
  std::unique_lock<std::mutex> lock(_mutex);
  std::vector<Peer> res;

  for (const auto &[_, peer] : _peers) {
    res.push_back(*peer);
  }

  return res;
}

void Peers::update_unreachable() {
  auto unreachables = by_status(Peer::UNREACHABLE);
  for (auto *peer : unreachables) {
    peer->update_unreachable(::neuro::time());
  }
}

std::optional<Peer *> Peers::peer_by_port(const Port port) const {
  std::unique_lock<std::mutex> lock(_mutex);
  for (auto &[_, peer] : _peers) {
    if (peer->port() == port) {
      return std::make_optional(peer.get());
    }
  }

  return std::nullopt;
}

bool Peers::fill(_Peers *peers, uint8_t peer_count) {
  const auto full_mask =
      static_cast<Peer::Status>(_Peer_Status_Status_MAX * 2 - 1);

  for (auto it = begin(full_mask), e = end(); it != e; ++it) {
    if (peer_count <= 0) {
      break;
    }
    peers->add_peers()->CopyFrom(**it);
    peer_count--;
  }
  return true;
}

Peers::operator _Peers() const {
  _Peers peers;
  for (const auto &[foo, peer] : _peers) {
    peers.add_peers()->CopyFrom(*peer.get());
  }
  return peers;
}

std::ostream &operator<<(std::ostream &os, const Peers &peers) {
  for (const auto &peer : peers.peers_copy()) {
    // os << "peers> "
    //<< "(" << peer.port() << ", " << _Peer_Status_Name(peer.status()) << ")"
    //<< std::endl;
    os << "peers> " << peer << std::endl;
  }

  return os;
}

}  // namespace messages
}  // namespace neuro
