#ifndef NEURO_SRC_NETWORKING_PEERPOOL_HPP
#define NEURO_SRC_NETWORKING_PEERPOOL_HPP

#include <random>
#include <unordered_map>

#include "common.pb.h"
#include "common/Buffer.hpp"
#include "crypto/EccPub.hpp"
#include "messages.pb.h"

namespace neuro {
namespace networking {

class PeerPool {
 public:
  using PeerPtr = std::shared_ptr<const messages::Peer>;

 private:
  using PeersPtr = std::unordered_map<std::size_t, PeerPtr>;

 private:
  std::string _path;
  PeersPtr _peers;
  std::optional<std::size_t> _my_key_pub_hash;
  std::size_t _max_size;
  mutable std::random_device _rd;
  mutable std::mt19937 _gen;
  mutable std::mutex _peers_mutex;

 private:
  static std::size_t hash(const crypto::EccPub& ecc_pub);

 private:
  bool insert(const PeerPtr& peer);
  bool prune();
  bool load();
  void save() const;
  void set(const messages::Peers& peers);
  messages::Peers build_peers_message() const;

 public:
  PeerPool(const std::string& path, const crypto::EccPub& my_ecc_pub,
           std::size_t max_size = 999);
  PeerPool(const std::string& path, std::size_t max_size = 999);
  void insert(const messages::Peers& peers);
  bool erase(const Buffer& key_pub);
  std::size_t size() const;
  std::optional<PeerPtr> get_random_not_of(
      const std::set<const Buffer*>& forbidden) const;
  std::set<PeerPtr> get_peers(const std::set<const Buffer*>& ids) const;
  std::set<PeerPtr> get_peers() const;
  messages::Peers peers_message() const;
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_PEERPOOL_HPP */
