#include "messages/Peers.hpp"

namespace neuro {
namespace messages {

Peers::iterator Peers::begin(const Peer::Status status) {
  return iterator{_peers, status};
}

Peers::iterator Peers::end() { return iterator{}; }

std::optional<Peer *> Peers::insert(const Peer &peer) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  if (peer.key_pub() == _own_key) {
    return {};
  }

  auto got = _peers.emplace(
      std::piecewise_construct, std::forward_as_tuple(peer.key_pub()),
      std::forward_as_tuple(std::make_unique<Peer>(peer)));

  if (!got.second) {
    // pub key already known, update peer
    //got.first->second->CopyFrom(peer);
    return {got.first->second.get()};
  }
  auto found_peer = got.first->second.get();
  got.first->second->set_status(Peer::DISCONNECTED);
  return found_peer;
}

// Peer *Peers::insert(const messages::KeyPub &key_pub,
//                     const std::optional<Endpoint> &endpoint,
//                     const std::optional<Port> &listen_port) {
//   std::unique_lock<std::shared_mutex> lock(_mutex);
//   auto it = std::find_if(
//       mutable_peers()->begin(), mutable_peers()->end(),
//       [&key_pub](const Peer &peer) { return (peer.key_pub() == key_pub); });
//   if (it != mutable_peers()->end()) {
//     return &(*it);
//   }

//   auto new_peer = add_peers();
//   LOG_TRACE << "incr peers " << peers().size();
//   new_peer->set_status(messages::Peer::DISCONNECTED);
//   if (endpoint) {
//     new_peer->set_endpoint(*endpoint);
//   }
//   if (listen_port) {
//     new_peer->set_port(*listen_port);
//   }
//   set_next_update(new_peer);
//   return new_peer;
// }

std::size_t Peers::used_peers_count() const {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return std::count_if(_peers.begin(), _peers.end(), [this](const auto &it) {
    const auto &peer = it.second;
    return (peer->has_status() && _used_status & peer->status());
  });
}

bool Peers::update_peer_status(const Peer &peer, const Peer::Status status) {
  LOG_TRACE;
  std::unique_lock<std::shared_mutex> lock(_mutex);
  auto got = _peers.find(peer.key_pub());
  if (got == _peers.end()) {
    return false;
  }
  got->second->set_status(status);
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
  LOG_TRACE;
  std::shared_lock<std::shared_mutex> lock(_mutex);
  std::vector<Peer *> res;

  const auto time = ::neuro::time();

  for (auto &pair : _peers) {
    auto *peer = pair.second.get();
    peer->update_unreachable(time);
    if (peer->status() & status) {
      res.push_back(peer);
    }
  }

  return res;
}

std::vector<Peer *> Peers::used_peers() {
  LOG_TRACE;
  return by_status(Peer::Status(_used_status));
}

std::vector<Peer *> Peers::connected_peers() {
  LOG_TRACE;
  return by_status(Peer::CONNECTED);
}

std::vector<Peer> Peers::peers_copy() const {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  std::vector<Peer> res;

  for (const auto &pair : _peers) {
    const auto &peer = pair.second.get();
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

bool Peers::fill(_Peers *peers) {
  const auto full_mask =
      static_cast<Peer::Status>(_Peer_Status_Status_MAX * 2 - 1);

  uint8_t peer_count = 10;  // TODO conf
  for (auto it = begin(full_mask), e = end(); it != e; ++it) {
    if (!(peer_count-- > 0)) {
      break;
    }
    peers->add_peers()->CopyFrom(**it);
  }
  return true;
}

std::ostream &operator<<(std::ostream &os, const Peers &peers) {
  for (const auto &peer : peers.peers_copy()) {
    os << "peers> " << peer << std::endl;
    ;
  }

  return os;
}

}  // namespace messages
}  // namespace neuro
