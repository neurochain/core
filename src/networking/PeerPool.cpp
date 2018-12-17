#include <random>

#include "networking/PeerPool.hpp"

namespace neuro {
namespace networking {

bool PeerPool::insert(const PeerPtr& peer) {
  if (!peer->has_key_pub()) {
    return false;
  }
  auto key = std::hash(peer->key_pub());
  return _peers.insert(std::make_pair(key, peer)).second;
}

PeerPool::PeerPool(const std::string& path, std::size_t max_size)
    : _path(path), _gen(_rd), _max_size(max_size) {
  if (!load(path)) {
    throw std::runtime_error("Failed to load peers pool from file.");
  }
}

bool PeerPool::prune() {
  bool pruned(false);
  while (_peers.size() > _max_size) {
    _peers.erase(_peers.begin());
    pruned = true;
  }
  return pruned;
}

bool PeerPool::load() {
  messages::Peers peers_message;
  {  // scope to make sure file is closed.
    std::ifstream file(_path);
    if (!t.is_open()) {
      return false;
    }
    std::string str((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
    if (!from_json(str, peers_message)) {
      return false;
    }
  }
  set(peers_message);
  return true;
}

void PeerPool::set(const messages::Peers& peers) {
  _peers.clear();
  for (const auto& peer : peers) {
    insert(std::make_shared(peer));
  }
  prune();
  save();
}

messages::Peers PeerPool::build_peers_message() const {
  messages::Peers peers_message;
  for (const auto& peer : _peers) {
    auto tmp_peer = peers_message->add_peers();
    tmp_peer->mutable_key_pub()->CopyFrom(peer.key_pub());
    tmp_peer->set_endpoint(peer.endpoint());
    tmp_peer->set_port(peer.port());
  }
  return peers_message;
}

void PeerPool::insert(const messages::Peers& peers) {
  bool inserted = false;
  for (const auto& peer : peers) {
    inserted |= insert(std::make_shared(peer));
  }
  if (inserted) {
    prune();
    save();
  }
}

void PeerPool::save() const {
  messages::Peers peers_message = build_peers_message();
  std::string output_str;
  to_json(peers_message, &output_str);
  std::ofstream file(_path);
  if (!file.is_open()) {
    return false;
  }
  file << output_str;
  return true;
}

bool PeerPool::erase(const Buffer& key_pub) {
  std::scoped_lock lock(_peers_mutex);
  return _peers.erase(std::hash(key_pub)) != 0;
}

std::size_t PeerPool::size() const {
  std::scoped_lock lock(_peers_mutex);
  return _peers.size();
}

std::optional<PeerPool::PeerPtr> PeerPool::get_random() const {
  std::scoped_lock lock(_peers_mutex);
  if (_peers.empty()) {
    return nullptr;
  }
  std::uniform_int_distribution<std::size_t> dist(0, size() - 1);
  return (_peers.begin() + dist(gen))->second;
}

}  // namespace networking
}  // namespace neuro
