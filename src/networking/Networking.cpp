#include <cassert>
#include <limits>
#include <string>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "crypto/Ecc.hpp"
#include "common/logger.hpp"
#include "networking/Networking.hpp"

namespace neuro {
namespace networking {

Networking::Networking(messages::Queue* queue,
                       messages::Peers *peers,
                       messages::config::Networking *config)
  : _queue(queue), _dist(0, std::numeric_limits<uint32_t>::max()) {

  load_keys(*config);
  _transport_layer = std::make_unique<Tcp>(config->tcp().port(), queue, peers, _keys);
  _queue->run();
}

bool Networking::load_keys(const messages::config::Networking &config) {
  bool keys_save{false};
  bool keys_create{false};

  std::string keypath_priv;
  std::string keypath_pub;

  if (config.has_key_priv_path() && config.has_key_pub_path()) {
    keypath_priv = config.key_priv_path();
    keypath_pub = config.key_pub_path();
  }

  if (keypath_pub.empty() && keypath_priv.empty()) {
    keys_create = true;
  } else if (!boost::filesystem::exists(keypath_pub) ||
             !boost::filesystem::exists(keypath_priv)) {
    keys_create = true;
    keys_save = true;
  }

  if (!keys_create) {
    LOG_INFO << this << " Loading keys from " << keypath_priv << " and "
             << keypath_pub;
    _keys = std::make_shared<crypto::Ecc>(keypath_priv, keypath_pub);
  } else {
    LOG_INFO << this << " Generating new keys";
    _keys = std::make_shared<crypto::Ecc>();
    if (keys_save) {
      LOG_INFO << this << " Saving keys to " << keypath_priv << " and "
               << keypath_pub;
      _keys->save(keypath_priv, keypath_pub);
    }
  }

  return true;
}

TransportLayer::SendResult Networking::send(
    std::shared_ptr<messages::Message> message) const {
  message->mutable_header()->set_id(_dist(_rd));
  return _transport_layer->send(message);
}

bool Networking::send_unicast(std::shared_ptr<messages::Message> message) const {
  return _transport_layer->send_unicast(message);
}

std::size_t Networking::peer_count() const {
  return _transport_layer->peer_count();
}
  
void Networking::join() { _transport_layer->join(); }

bool Networking::terminate(const Connection::ID id) {
  return _transport_layer->terminate(id);
}

Port Networking::listening_port() const {
  return _transport_layer->listening_port();
}

bool Networking::connect(messages::Peer * peer) {
  return _transport_layer->connect(peer);
}


Networking::~Networking() { LOG_DEBUG << this << " Networking killed"; }

}  // namespace networking
}  // namespace neuro
