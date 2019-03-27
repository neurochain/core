#include "messages/Peers.hpp"

namespace neuro {
namespace messages {

Peers::iterator Peers::begin(const Peer::Status status) {
  return iterator{_peers, status};
}

Peers::iterator Peers::begin() {
  return iterator{_peers};
}

Peers::iterator Peers::end() { return iterator{}; }

std::optional<Peer *> Peers::insert(const Peer &peer) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  if (peer.key_pub() == _own_key) {
    return {};
  }

  auto [found_element, is_inserted] = _peers.emplace(
      std::piecewise_construct, std::forward_as_tuple(peer.key_pub()),
      std::forward_as_tuple(std::make_unique<Peer>(peer)));

  auto& found_peer = found_element->second;

  if (!is_inserted) {
    // pub key already known, update peer
    found_peer->set_port(peer.port());
    found_peer->set_endpoint(peer.endpoint());
    return {found_peer.get()};
  }
  found_peer->set_status(Peer::DISCONNECTED);
  return found_peer.get();
}

/**
 * count the number of bot communicating with us (connecting or connected peers)
 * @return number of connection | connected peers
 */
std::size_t Peers::used_peers_count() const {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  long used_peers_count = std::count_if(_peers.begin(), _peers.end(), [](const auto &it) {
    const auto &peer = it.second;
    auto connection_status = peer->status() & (Peer::CONNECTING | Peer::CONNECTED);
    return peer->has_status() && connection_status;
  });
  return used_peers_count;
}

bool Peers::update_peer_status(const Peer &peer, const Peer::Status status) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  auto found_peer = _peers.find(peer.key_pub());
  if (found_peer == _peers.end()) {
    return false;
  }
  found_peer->second->set_status(status);
  return true;
}

std::optional<Peer *> Peers::find(const KeyPub &key_pub) {
  auto got = _peers.find(key_pub);

  if (got == _peers.end()) {
    return std::nullopt;
  }

  return {got->second.get()};
}

std::vector<Peer *> Peers::by_status(const Peer::Status status) {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  std::vector<Peer *> res;

  const auto time = ::neuro::time();

  for (auto& [_, peer] : _peers) {
    peer->update_unreachable(time);
    if (peer->status() & status) {
      res.push_back(peer.get());
    }
  }

  return res;
}

std::vector<Peer *> Peers::used_peers() {
  return by_status(static_cast<Peer::Status>(Peer::CONNECTING | Peer::CONNECTED));
}

std::vector<Peer *> Peers::connected_peers() {
  return by_status(Peer::CONNECTED);
}

std::vector<Peer> Peers::peers_copy() const {
  std::shared_lock<std::shared_mutex> lock(_mutex);
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

std::ostream &operator<<(std::ostream &os, const Peers &peers) {
  for (const auto &peer : peers.peers_copy()) {
    os << "peers> " << peer << std::endl;
  }

  return os;
}

}  // namespace messages
}  // namespace neuro
