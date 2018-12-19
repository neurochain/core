#include <random>

#include "common/Buffer.hpp"
#include "common/logger.hpp"
#include "crypto/EccPub.hpp"
#include "messages/Message.hpp"
#include "networking/PeerPool.hpp"

namespace neuro {
namespace networking {

std::size_t PeerPool::hash(const crypto::EccPub& ecc_pub) {
  const neuro::Buffer buffer = ecc_pub.save();
  return std::hash<neuro::Buffer>{}(buffer);
}

bool PeerPool::insert(const PeerPtr& peer) {
  if (!peer->has_key_pub()) {
    return false;
  }
  crypto::EccPub ecc_pub;
  ecc_pub.load(peer->key_pub());
  auto key = hash(ecc_pub);
  if (!!_my_key_pub_hash && key == *_my_key_pub_hash) {
    return false;
  }
  return _peers.insert(std::make_pair(key, peer)).second;
}

PeerPool::PeerPool(const std::string& path, const crypto::EccPub& my_ecc_pub,
                   std::size_t max_size)
    : _path(path),
      _my_key_pub_hash(hash(my_ecc_pub)),
      _max_size(max_size),
      _gen(_rd()) {
  if (!load()) {
    throw std::runtime_error("Failed to load peers pool from file.");
  }
}

PeerPool::PeerPool(const std::string& path, std::size_t max_size)
    : _path(path), _max_size(max_size), _gen(_rd()) {
  if (!load()) {
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
    if (!file.is_open()) {
      return false;
    }
    std::string str((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
    if (!from_json(str, &peers_message)) {
      return false;
    }
  }
  set(peers_message);
  return true;
}

void PeerPool::set(const messages::Peers& peers) {
  _peers.clear();
  const auto& iterable_peers = peers.peers();
  for (const auto& peer : iterable_peers) {
    insert(std::make_shared<messages::Peer>(peer));
  }
  prune();
  save();
}

messages::Peers PeerPool::build_peers_message() const {
  messages::Peers peers_message;
  for (const auto& peer_it : _peers) {
    auto tmp_peer = peers_message.add_peers();
    tmp_peer->mutable_key_pub()->CopyFrom(peer_it.second->key_pub());
    tmp_peer->set_endpoint(peer_it.second->endpoint());
    tmp_peer->set_port(peer_it.second->port());
  }
  return peers_message;
}

void PeerPool::insert(const messages::Peers& peers) {
  bool inserted = false;
  const auto& iterable_peers = peers.peers();
  for (const auto& peer : iterable_peers) {
    inserted |= insert(std::make_shared<messages::Peer>(peer));
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
    LOG_ERROR << "Failed to save peers list to file.";
    return;
  }
  file << output_str;
}

bool PeerPool::erase(const Buffer& key_pub) {
  std::scoped_lock lock(_peers_mutex);
  return _peers.erase(std::hash<Buffer>{}(key_pub)) != 0;
}

std::size_t PeerPool::size() const {
  std::scoped_lock lock(_peers_mutex);
  return _peers.size();
}

std::optional<PeerPool::PeerPtr> PeerPool::get_random_not_of(
    const std::set<const Buffer*>& forbidden) const {
  std::scoped_lock lock(_peers_mutex);
  if (_peers.empty()) {
    return nullptr;
  }
  std::set<std::size_t> forbidden_keys;
  for (const auto& f : forbidden) {
    assert(f != nullptr);
    forbidden_keys.insert(std::hash<Buffer>{}(*f));
  }
  std::vector<std::size_t> eligible;
  eligible.reserve(_peers.size());
  for (const auto& peer : _peers) {
    if (forbidden_keys.find(peer.first) == forbidden_keys.end()) {
      eligible.push_back(peer.first);
    }
  }
  std::uniform_int_distribution<std::size_t> dist(0, eligible.size() - 1);
  auto selected_key = eligible[dist(_gen)];
  return _peers.at(selected_key);
}

std::set<PeerPool::PeerPtr> PeerPool::get_peers(
    const std::set<const Buffer*>& ids) const {
  std::set<PeerPtr> ans;
  std::scoped_lock lock(_peers_mutex);
  for (const auto buff : ids) {
    assert(buff != nullptr);
    auto key = std::hash<neuro::Buffer>{}(*buff);
    auto peer_it = _peers.find(key);
    if (peer_it == _peers.end()) {
      LOG_ERROR << "Peer not found!";
    } else {
      ans.insert(peer_it->second);
    }
  }
  return ans;
}

std::set<PeerPool::PeerPtr> PeerPool::get_peers() const {
  std::set<PeerPtr> ans;
  std::scoped_lock lock(_peers_mutex);
  for (const auto& peer_it : _peers) {
    ans.insert(peer_it.second);
  }
  return ans;
}

messages::Peers PeerPool::peers_message() const {
  std::scoped_lock lock(_peers_mutex);
  return build_peers_message();
}

}  // namespace networking
}  // namespace neuro
