#include "Bot.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "messages/Subscriber.hpp"
#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <cstdlib>
#include <ctime>
#include <random>

namespace neuro {

Bot::Bot(std::istream &bot_stream)
    : _queue(std::make_shared<messages::Queue>()),
      _networking(std::make_shared<networking::Networking>(_queue)),
      _subscriber(std::make_shared<messages::Subscriber>(_queue)) {
  std::string tmp;
  bot_stream >> tmp;
  messages::from_json(tmp, &_config);
  if (!init()) {
    throw std::runtime_error("Could not create bot from configuration file");
  }
}

Bot::Bot(const std::string &configuration_path)
    : _queue(std::make_shared<messages::Queue>()),
      _networking(std::make_shared<networking::Networking>(_queue)),
      _subscriber(std::make_shared<messages::Subscriber>(_queue)) {
  messages::from_json_file(configuration_path, &_config);
  if (!init()) {
    throw std::runtime_error("Could not create bot from configuration file");
  }
}

bool Bot::init() {
  if (_config.has_logs()) {
    log::from_config(_config.logs());
  }
  const auto keypath_priv = _config.key_priv_path();
  const auto keypath_pub = _config.key_pub_path();
  bool keys_save{false};
  bool keys_create{false};

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

  const auto &networking_conf = _config.networking();
  _selection_method = _config.selection_method();
  _keep_status = _config.keep_status();
  _max_connections = networking_conf.max_connections();

  if (networking_conf.has_tcp()) {
    _tcp_config = networking_conf.tcp();
    _tcp = std::make_shared<networking::Tcp>(_queue, _keys);
    auto port = _tcp_config.listen_port();
    _tcp->accept(port);
    LOG_INFO << this << " Accepting connections on port " << port;
    _networking->push(_tcp);
    auto peers = _tcp_config.peers(); // const
    if (peers.empty()) {
      LOG_WARNING << this << " There is no information about peers";
    }
  }

  _subscriber->subscribe(
      messages::Type::kHello,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_hello(header, body);
      });

  _subscriber->subscribe(
      messages::Type::kWorld,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_world(header, body);
      });

  _subscriber->subscribe(
      messages::Type::kConnectionClosed,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_deconnection(header, body);
      });

  _subscriber->subscribe(
      messages::Type::kConnectionReady,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_connection(header, body);
      });
  return true;
}

void Bot::handler_connection(const messages::Header &header,
                             const messages::Body &body) {
  if (!body.has_connection_ready()) {
    LOG_WARNING << this
                << "Got a call to handler_connection with "
                   "different type of body on the msg";
    return;
  }

  if (!header.has_peer()) {
    // TODO: ask to close the connection
    LOG_ERROR << this
              << " The header does not have a peer. Ignoring connection";
    return;
  }

  auto peer = header.peer();
  auto connection_ready = body.connection_ready();

  if (!connection_ready.from_remote()) {
    LOG_DEBUG << this << " Got a connection to " << peer;

    // send the hello msg
    auto message = std::make_shared<messages::Message>();
    message->mutable_header()->mutable_peer()->CopyFrom(peer);
    auto hello = message->add_bodies()->mutable_hello();
    hello->set_listen_port(_tcp->listening_port());

    Buffer pub_key_buffer;
    _keys->public_key().save(&pub_key_buffer);

    auto key_pub = hello->mutable_key_pub();
    key_pub->set_type(messages::KeyType::ECP256K1);
    key_pub->set_hex_data(pub_key_buffer.str());

    _networking->send_unicast(message, networking::ProtocolType::PROTOBUF2);
  } else {
    // Nothing to do; just wait for the hello message from remote peer
    LOG_DEBUG << this << " Got a connection from " << peer;
  }
}

void Bot::handler_deconnection(const messages::Header &header,
                               const messages::Body &body) {

  if (!body.has_connection_closed()) {
    LOG_WARNING << this
                << " SomeThing wrong. Got a call to handler_deconnection with "
                   "different type of body on the msg";
    return;
  }
  // auto connection_closed = body.connection_closed();
  LOG_DEBUG << this << " Got a connection_closed message";

  auto remote_peer = header.peer();
  // find the peer in our list of peers and update its status
  auto peers = _tcp_config.mutable_peers();
  auto it =
      std::find_if(peers->begin(), peers->end(), [remote_peer](const auto &el) {
        if (remote_peer.endpoint() == el.endpoint()) {
          if (remote_peer.has_port() && el.has_port()) {
            return remote_peer.port() == el.port();

          } else {
            return !remote_peer.has_port() && !el.has_port();
          }
        }
        return false;
      });

  _tcp->terminated(remote_peer.connection_id());

  using ps = messages::Peer_Status;
  auto old_status = remote_peer.status();

  if (old_status == ps::Peer_Status_CONNECTING && it != peers->end()) {
    (*it).set_status(ps::Peer_Status_UNREACHABLE);
  } else if (old_status == ps::Peer_Status_CONNECTED) {
    _connected_peers -= 1;
  }

  this->keep_max_connections();
}

void Bot::handler_world(const messages::Header &header,
                        const messages::Body &body) {
  if (!body.has_world()) {
    LOG_WARNING << this
                << " SomeThing wrong. Got a call to handler_world with "
                   "different type of body on the msg";
    return;
  }
  auto world = body.world();

  LOG_DEBUG << this << " Got a WORLD message";
  using ps = messages::Peer_Status;

  if (world.peers().size() > 0) {
    std::stringstream ss;
    ss << world.peers();

    LOG_DEBUG << this << " Peers from remote: " << ss.str();
  }

  // Check that I am not in the received list
  auto peers = _tcp_config.mutable_peers();
  for (const auto &peer : world.peers()) {
    auto predicate = [peer, this](const auto &it) {
      if (it.endpoint() == this->_tcp->local_ip().to_string()) {
        if (it.has_port()) {
          return it.port() == this->_tcp->listening_port();
        }
      }
      return false;
    };

    if (std::find_if(peers->begin(), peers->end(), predicate) != peers->end()) {
      continue;
    }

    _tcp_config.add_peers()->CopyFrom(peer);
  }

  if (!header.has_peer()) {
    LOG_ERROR << this << " Got an empty peer. Going out of the world handler";
    return;
  }

  auto peer_header = header.peer();

  // we should have it in our unconnected list because we told him "hello"
  auto peer_it = std::find_if(
      peers->begin(), peers->end(), [&peer_header](const auto &it) {
        return it.endpoint() == peer_header.endpoint() &&
               it.port() == peer_header.port();
      });

  messages::Peer *remote_peer;
  if (peer_it == peers->end()) {
    remote_peer = _tcp_config.add_peers();
    remote_peer->CopyFrom(peer_header);
  } else {
    remote_peer = &(*peer_it);
  }

  LOG_DEBUG << this << " The WORLD message is from " << *remote_peer;

  if (!world.accepted()) {
    LOG_DEBUG << this << " Not accepted, disconnecting ...";
    remote_peer->set_status(ps::Peer_Status_FULL);
    // Should I call terminate from the connection
    _tcp->terminated(remote_peer->connection_id());
  } else {
    bool accepted{false};
    {
      std::lock_guard<std::mutex> lock_connections(_mutex_connections);
      accepted = _connected_peers < _max_connections;
      if (accepted) {
        _connected_peers += 1;
      }
    }

    if (!accepted) {
      LOG_DEBUG << this << " Closing a connection because I am already full";
      remote_peer->set_status(ps::Peer_Status_REACHABLE);
      _tcp->terminated(remote_peer->connection_id());
    } else {
      remote_peer->set_status(ps::Peer_Status_CONNECTED);
    }
  }

  this->keep_max_connections();
}

void Bot::handler_hello(const messages::Header &header,
                        const messages::Body &body) {
  if (!body.has_hello()) {
    LOG_WARNING << this
                << " SomeThing wrong. Got a call to handler_hello with "
                   "different type of body on the msg";
    return;
  }
  auto hello = body.hello();

  LOG_DEBUG << this << " Got a HELLO message";
  using ps = messages::Peer_Status;

  // == Create world message for replying ==
  auto message = std::make_shared<messages::Message>();
  auto world = message->add_bodies()->mutable_world();
  bool accepted, peers_connected{false};
  {
    std::lock_guard<std::mutex> lock_connections(_mutex_connections);
    accepted = _connected_peers < _max_connections;
    peers_connected = _connected_peers > 0;
    if (accepted) {
      _connected_peers += 1;
    }
  }

  auto peers = _tcp_config.peers();
  if (peers_connected) {
    for (const auto &peer_conn : peers) {
      if (peer_conn.status() == ps::Peer_Status_CONNECTED ||
          peer_conn.status() == ps::Peer_Status_REACHABLE) {
        world->add_peers()->CopyFrom(peer_conn);
      }
    }
  }
  message->mutable_header()->CopyFrom(header);
  _networking->send_unicast(message, networking::ProtocolType::PROTOBUF2);

  // == Check if we need to add this peer to our list or not ==
  auto peer_header = header.peer();
  auto peer_it = std::find_if(
      peers.begin(), peers.end(), [&peer_header, &hello](const auto &it) {
        return it.endpoint() == peer_header.endpoint() &&
               it.port() == hello.listen_port();
      });
  bool found = peer_it != peers.end();

  messages::Peer *remote_peer;
  if (!found) {
    remote_peer = _tcp_config.add_peers();
    remote_peer->CopyFrom(peer_header);
  } else {
    remote_peer = &(*peer_it);
  }

  if (accepted) {
    remote_peer->set_status(ps::Peer_Status_CONNECTED);
  } else {
    remote_peer->set_status(ps::Peer_Status_REACHABLE);
  }
}

std::ostream &operator<<(
    std::ostream &os,
    const google::protobuf::RepeatedPtrField<neuro::messages::Peer> &peers) {
  os << "{";
  for (const auto &p : peers) {
    os << p;
  }
  os << "}";
  return os;
}

std::ostream &operator<<(std::ostream &os, const neuro::Bot &b) {
  auto status = b.status();
  os << "Bot(" << &b << ") { connected: " << status.connected_peers
     << ", max_connections: " << status.max_connections
     << ", peers in pool: " << status.peers_in_pool << " }";
  return os;
}

Bot::Status Bot::status() const {
  std::lock_guard<std::mutex> lock_connections(_mutex_connections);
  auto peers_size = _tcp_config.peers().size();
  auto status = Bot::Status(_connected_peers, _max_connections, peers_size);
  return status;
}

bool Bot::next_to_connect(messages::Peer **peer) {
  using ps = messages::Peer_Status;
  using sm = messages::config::Config_NextSelectionMethod;
  auto peers = _tcp_config.mutable_peers();

  switch (_selection_method) {
  case sm::Config_NextSelectionMethod_SIMPLE: {
    //
    auto it = std::find_if(peers->begin(), peers->end(), [](const auto &el) {
      return el.status() == ps::Peer_Status_REACHABLE;
    });

    if (it != peers->end()) {
      *peer = &(*it);
      return true;
    }
  } break;
  case sm::Config_NextSelectionMethod_PING: {
    LOG_WARNING << this
                << " SelectionMethod::PING is not implemented - Using RANDOM ";
  } // break; // TODO: After implementing PING, remove the comment from break
  case sm::Config_NextSelectionMethod_RANDOM: {
    // Create a vector with all possible positions shuffled
    std::vector<std::size_t> pos((std::size_t)peers->size());
    std::iota(pos.begin(), pos.end(), 0);
    std::srand(unsigned(std::time(0)));
    std::random_shuffle(pos.begin(), pos.end());

    // Check every pos until we find one that is good to use
    for (const auto &idx : pos) {
      auto tmp_peer = peers->Mutable(idx);
      //auto &tmp_peer = peers[idx];
      if (tmp_peer->status() == ps::Peer_Status_REACHABLE) {
        *peer = tmp_peer;
        return true;
      }
    }
  } break;
  default:
    LOG_ERROR << this
              << " Uknown method for selecting next peer to connect to ";
  }
  return false;
}

void Bot::keep_max_connections() {
  using ps = messages::Peer_Status;
  auto peers = _tcp_config.mutable_peers();
  if (peers->empty()) {
    LOG_INFO << this << " Stayng server mode";
    return;
  }
  {
    std::lock_guard<std::mutex> lock_connections(_mutex_connections);
    if (_connected_peers == _max_connections)
      return;

    if (_connected_peers == (std::size_t)peers->size()) {
      LOG_WARNING << this << " There is no available peer to check";
      return;
    }

    if (_connected_peers < _max_connections) {
      messages::Peer *peer;
      if (this->next_to_connect(&peer)) {
        LOG_DEBUG << this << " Asking to connect to " << *peer;
        peer->set_status(ps::Peer_Status_CONNECTING);
        auto tmp_peer = std::make_shared<messages::Peer>();
        tmp_peer->CopyFrom(*peer);
        _tcp->connect(tmp_peer);
      } else {
        LOG_DEBUG << this << " No more REACHABLE peers";
      }
    }
  }
}

void Bot::join() { this->keep_max_connections(); }

Bot::~Bot() {}

} // namespace neuro
