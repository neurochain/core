#include "messages/Peers.hpp"

namespace neuro {
namespace messages {

Peer *Peers::insert(const Peer &peer) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  // auto pair = std::make_pair(peer.key_pub(), );
  auto got = _peers.emplace(
      std::piecewise_construct, std::forward_as_tuple(peer.key_pub()),
      std::forward_as_tuple(std::make_unique<Peer>(peer)));
  auto found_peer = got.first->second.get();
  if (!found_peer->has_status()) {
    found_peer->set_status(Peer::DISCONNECTED);
  }
  found_peer->update_timestamp();
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
  LOG_TRACE;
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return std::count_if(_peers.begin(), _peers.end(), [this](const auto &it) {
    const auto &peer = it.second;
    if (peer->has_status()) {
      std::cout << "used " << *peer << " " << (_used_status & peer->status())
                << std::endl;
    };
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

std::optional<Peer *> Peers::next(const Peer::Status status) {
  LOG_TRACE;
  std::shared_lock<std::shared_mutex> lock(_mutex);
  const auto current_time = std::time(nullptr);
  for (const auto &pair : _peers) {
    auto peer = pair.second.get();
    
    if (peer->has_status() && peer->status() & status &&
        (current_time > peer->next_update().data())) {
      return {peer};
    }
  }

  return std::nullopt;
}

std::vector<Peer *> Peers::by_status(const Peer::Status status) const {
  LOG_TRACE;
  std::shared_lock<std::shared_mutex> lock(_mutex);
  std::vector<Peer *> res;

  for (const auto &pair : _peers) {
    const auto &peer = pair.second.get();
    if (peer->status() & status) {
      res.push_back(peer);
    }
  }

  return res;
}

std::vector<Peer *> Peers::used_peers() const {
  LOG_TRACE;
  return by_status(Peer::Status(_used_status));
}

std::vector<Peer *> Peers::connected_peers() const {
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
    if(peer->next_update().data() < std::time(nullptr)) {
      peer->set_status(Peer::DISCONNECTED);
    }
  }
}

bool Peers::fill(_Peers *peers) {
  const auto full_mask = _Peer_Status_Status_MAX * 2 - 1;
  for (int i = 0; i < 10; i++) {
    auto peer = next(static_cast<Peer::Status>(full_mask));
    if (!peer) {
      return false;
    }
    peers->add_peers()->CopyFrom(**peer);
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
