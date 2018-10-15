#include "Bot.hpp"
#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <cstdlib>
#include <ctime>
#include <random>
#include "common/logger.hpp"
#include "common/types.hpp"
#include "consensus/PiiConsensus.hpp"
#include "messages/Subscriber.hpp"

namespace neuro {

Bot::Bot(std::istream &bot_stream)
    : _queue(std::make_shared<messages::Queue>()),
      _networking(std::make_shared<networking::Networking>(_queue)),
      _subscriber(_queue),
      _io_context(std::make_shared<boost::asio::io_context>()) {
  std::string tmp;
  std::stringstream ss;
  while (!bot_stream.eof()) {
    bot_stream >> tmp;
    ss << tmp;
  }
  tmp = ss.str();
  messages::from_json(tmp, &_config);
  if (!init()) {
    throw std::runtime_error("Could not create bot from configuration file");
  }
}

Bot::Bot(const std::string &configuration_path)
    : _queue(std::make_shared<messages::Queue>()),
      _networking(std::make_shared<networking::Networking>(_queue)),
      _subscriber(_queue),
      _io_context(std::make_shared<boost::asio::io_context>()) {
  if (!messages::from_json_file(configuration_path, &_config)) {
    std::string s = "Coult not parse configuration file " + configuration_path +
                    " from " + boost::filesystem::current_path().native();
    throw std::runtime_error(s);
  }
  if (!init()) {
    throw std::runtime_error("Could not create bot from configuration file");
  }
}

bool Bot::load_keys(const messages::config::Config &config) {
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

void Bot::handler_get_block(const messages::Header &header,
                            const messages::Body &body) {
  LOG_DEBUG << this << " Got a get_block message";
  const auto get_block = body.get_block();

  auto message = std::make_shared<messages::Message>();
  auto header_reply = message->mutable_header();
  messages::fill_header_reply(header, header_reply);

  if (get_block.has_hash()) {
    LOG_ERROR << " get_block by hash not implemented";  // TODO
  } else if (get_block.has_height()) {
    const auto height = get_block.height();
    for (auto i = 0u; i < get_block.count(); ++i) {
      auto block = message->add_bodies()->mutable_block();
      _ledger->get_block(height + i, block);
    }
  } else {
    LOG_ERROR << this << " get_block message ill-formed";
    return;
  }

  _networking->send_unicast(message, networking::ProtocolType::PROTOBUF2);
}

void Bot::handler_block(const messages::Header &header,
                        const messages::Body &body) {
  _consensus->add_block(body.block());
  update_ledger();
}

void Bot::handler_transaction(const messages::Header &header,
                              const messages::Body &body) {
  // TODO send to concensus

  // update_ledger();
}

bool Bot::update_ledger() {
  messages::BlockHeader last_header;
  if (!_ledger->get_last_block_header(&last_header)) {
    LOG_ERROR << "Ledger should have at least block0";
    return false;
  }

  if ((last_header.timestamp().data() - time(nullptr)) < BLOCK_PERIODE) {
    return true;
  }

  auto message = std::make_shared<messages::Message>();
  auto header = message->mutable_header();
  messages::fill_header(header);

  const auto height = last_header.height();
  auto get_block = message->add_bodies()->mutable_get_block();
  get_block->set_height(height + 1);
  get_block->set_count(1);
  _networking->send(message, networking::ProtocolType::PROTOBUF2);

  return false;
}

void Bot::subscribe() {
  _subscriber.subscribe(
      messages::Type::kHello,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_hello(header, body);
      });

  _subscriber.subscribe(
      messages::Type::kWorld,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_world(header, body);
      });

  _subscriber.subscribe(
      messages::Type::kConnectionClosed,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_deconnection(header, body);
      });

  _subscriber.subscribe(
      messages::Type::kConnectionReady,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_connection(header, body);
      });

  _subscriber.subscribe(
      messages::Type::kTransaction,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_transaction(header, body);
      });

  _subscriber.subscribe(
      messages::Type::kBlock,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_block(header, body);
      });

  _subscriber.subscribe(
      messages::Type::kGetBlock,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_get_block(header, body);
      });
}

bool Bot::load_networking(messages::config::Config *config) {
  auto networking_conf = config->mutable_networking();
  _selection_method = config->selection_method();
  _keep_status = config->keep_status();
  _max_connections = networking_conf->max_connections();

  if (!networking_conf->has_tcp()) {
    LOG_ERROR << "Missing tcp configuration";
    return false;
  }

  auto tcpconfig = networking_conf->mutable_tcp();

  for (auto &peer : *tcpconfig->mutable_peers()) {
    peer.set_status(messages::Peer::REACHABLE);
    LOG_DEBUG << this << " Peer: " << peer;
  }

  _tcp = std::make_shared<networking::Tcp>(_queue, _keys);
  auto port = tcpconfig->port();
  _tcp->accept(port);
  LOG_INFO << this << " Accepting connections on port " << port;
  _networking->push(_tcp);
  if (tcpconfig->peers().empty()) {
    LOG_WARNING << this << " There is no information about peers";
  }

  return true;
}

bool Bot::init() {
  if (_config.has_logs()) {
    log::from_config(_config.logs());
  }

  _tcp_config = _config.mutable_networking()->mutable_tcp();

  load_keys(_config);
  if (!_config.has_database()) {
    LOG_ERROR << "Missing db configuration";
    return false;
  }
  subscribe();

  const auto db_config = _config.database();
  _ledger = std::make_shared<ledger::LedgerMongodb>(db_config);

  if (!_config.has_networking() || !load_networking(&_config)) {
    LOG_ERROR << "Could not load networking";
    return false;
  }

  _consensus = std::make_shared<consensus::PiiConsensus>(_io_context, _ledger, _networking);
  auto ecc = std::make_shared<crypto::Ecc>("keys/key_faucet.priv", "keys/key_faucet.pub");
  _consensus->add_wallet_keys(ecc);
  std::thread([this]() { _io_context->run(); }).detach();



  if (!_config.has_rest()) {
    LOG_INFO << "Missing rest configuration, not loading module";
  } else {
    const auto rest_config = _config.rest();
    _rest = std::make_shared<rest::Rest>(_ledger, _networking, _keys,
                                         _consensus, rest_config);
  }

  update_ledger();

  return true;
}

void Bot::handler_connection(const messages::Header &header,
                             const messages::Body &body) {
  if (!header.has_peer()) {
    // TODO: ask to close the connection
    LOG_ERROR << this
              << " The header does not have a peer. Ignoring connection";
    return;
  }

  auto peer = header.peer();
  auto connection_ready = body.connection_ready();

  if (connection_ready.from_remote()) {
    // Nothing to do; just wait for the hello message from remote peer
    LOG_DEBUG << this << " Got a connection from " << peer;
    return;
  }

  LOG_DEBUG << this << " Got a connection to " << peer;
  // send hello msg
  auto message = std::make_shared<messages::Message>();
  messages::fill_header(message->mutable_header());

  message->mutable_header()->mutable_peer()->CopyFrom(peer);
  auto hello = message->add_bodies()->mutable_hello();
  hello->set_listen_port(_tcp->listening_port());

  Buffer key_pub_buffer;
  _keys->public_key().save(&key_pub_buffer);

  auto key_pub = hello->mutable_key_pub();
  key_pub->set_type(messages::KeyType::ECP256K1);
  const auto tmp = _keys->public_key().save();
  key_pub->set_raw_data(tmp.data(), tmp.size());

  std::cout << this << " setting pub key in hello " << tmp << std::endl;

  _networking->send_unicast(message, networking::ProtocolType::PROTOBUF2);
}

void Bot::handler_deconnection(const messages::Header &header,
                               const messages::Body &body) {
  LOG_DEBUG << this << " Got a connection_closed message";

  auto remote_peer = header.peer();
  // find the peer in our list of peers and update its status
  // std::lock_guard<std::mutex> lock_connections(_mutex_connections);
  auto peers = _tcp_config->mutable_peers();
  auto it = std::find_if(peers->begin(), peers->end(),
                         [&remote_peer](const auto &el) {
                           if ((remote_peer.endpoint() != el.endpoint()) ||
                               !remote_peer.has_port() || !el.has_port()) {
                             return false;
                           }
                           return remote_peer.port() == el.port();
                         });

  _tcp->terminated(remote_peer.connection_id());

  auto old_status = remote_peer.status();

  if (it == peers->end()) {
    LOG_WARNING << "Unknown peer disconnected";
    this->keep_max_connections();

    return;
  }

  if (old_status == messages::Peer::CONNECTED) {
    _connected_peers--;
  }
  it->set_status(messages::Peer::UNREACHABLE);

  this->keep_max_connections();
}

void Bot::handler_world(const messages::Header &header,
                        const messages::Body &body) {
  auto world = body.world();
  LOG_DEBUG << this << " Got a WORLD message";

  // std::lock_guard<std::mutex> lock_connections(_mutex_connections);
  // Check that I am not in the received list
  auto peers = _tcp_config->mutable_peers();
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

    _tcp_config->add_peers()->CopyFrom(peer);
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
    remote_peer = _tcp_config->add_peers();
    remote_peer->CopyFrom(peer_header);
  } else {
    remote_peer = &(*peer_it);
  }

  LOG_DEBUG << this << " The WORLD message is from " << *remote_peer;

  if (!world.accepted()) {
    LOG_DEBUG << this << " Not accepted, disconnecting ...";
    remote_peer->set_status(messages::Peer::FULL);
    // Should I call terminate from the connection
    _tcp->terminated(remote_peer->connection_id());
  } else {
    bool accepted{false};
    {
      // std::lock_guard<std::mutex> lock_connections(_mutex_connections);
      accepted = _connected_peers < _max_connections;
      if (accepted) {
        _connected_peers++;
      }
    }

    if (!accepted) {
      LOG_DEBUG << this << " Closing a connection because I am already full";
      remote_peer->set_status(messages::Peer::REACHABLE);
      _tcp->terminated(remote_peer->connection_id());
    } else {
      remote_peer->set_status(messages::Peer::CONNECTED);
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

  // == Create world message for replying ==
  auto message = std::make_shared<messages::Message>();
  auto world = message->add_bodies()->mutable_world();
  bool accepted = _connected_peers < _max_connections;
  if (accepted) {
    _connected_peers += 1;
  }

  auto peers = _tcp_config->peers();
  for (const auto &peer_conn : peers) {
    if (peer_conn.status() == messages::Peer::CONNECTED ||
        peer_conn.status() == messages::Peer::REACHABLE) {
      world->add_peers()->CopyFrom(peer_conn);
    }
  }

  auto header_reply = message->mutable_header();
  messages::fill_header_reply(header, header_reply);
  world->set_accepted(accepted);

  Buffer key_pub_buffer;
  _keys->public_key().save(&key_pub_buffer);
  auto key_pub = world->mutable_key_pub();
  key_pub->set_type(messages::KeyType::ECP256K1);
  key_pub->set_hex_data(key_pub_buffer.str());

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
    remote_peer = _tcp_config->add_peers();
    remote_peer->CopyFrom(peer_header);
  } else {
    remote_peer = &(*peer_it);
  }

  if (accepted) {
    remote_peer->set_status(messages::Peer::CONNECTED);
  } else {
    remote_peer->set_status(messages::Peer::REACHABLE);
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
  // std::lock_guard<std::mutex> lock_connections(_mutex_connections);
  auto peers_size = _tcp_config->peers().size();
  auto status = Bot::Status(_connected_peers, _max_connections, peers_size);
  return status;
}

bool Bot::next_to_connect(messages::Peer **peer) {
  // it is locked from the caller
  auto peers = _tcp_config->mutable_peers();

  for (auto &peer : *_tcp_config->mutable_peers()) {
    std::cout << this << " PEER STATUS " << peer << std::endl;
  }

  switch (_selection_method) {
    case messages::config::Config::SIMPLE: {
      LOG_DEBUG << this << " It entered the simple method for next selection";
      auto it = std::find_if(peers->begin(), peers->end(), [](const auto &el) {
        return el.status() == messages::Peer::REACHABLE;
      });

      if (it == peers->end()) {
        LOG_DEBUG << this << " No reachable peer";
        return false;
      } else {
        *peer = &(*it);
        LOG_DEBUG << this << " found a peer";
        return true;
      }
      break;
    }
    case messages::config::Config::PING: {
      LOG_WARNING
          << this
          << " SelectionMethod::PING is not implemented - Using RANDOM ";
    }  // break; // TODO: After implementing PING, remove the comment from break
    case messages::config::Config::RANDOM: {
      LOG_DEBUG << this << " It entered the random method for next selection";
      // Create a vector with all possible positions shuffled
      std::vector<std::size_t> pos((std::size_t)peers->size());
      std::iota(pos.begin(), pos.end(), 0);
      std::srand(unsigned(std::time(0)));
      std::random_shuffle(pos.begin(), pos.end());

      // Check every pos until we find one that is good to use
      for (const auto &idx : pos) {
        auto tmp_peer = peers->Mutable(idx);
        // auto &tmp_peer = peers[idx];
        if (tmp_peer->status() == messages::Peer::REACHABLE) {
          *peer = tmp_peer;
          return true;
        }
      }
      break;
    }
    default:
      LOG_ERROR << this
                << " Uknown method for selecting next peer to connect to ";
  }
  return false;
}

void Bot::keep_max_connections() {
  std::size_t peers_size = 0;
  peers_size = _tcp_config->peers().size();

  if (peers_size == 0) {
    LOG_INFO << this << " No peers";
    return;
  }
  LOG_DEBUG << this << " peer count " << peers_size;

  if (_connected_peers == _max_connections) return;

  if (_connected_peers == peers_size) {
    LOG_WARNING << this << " No available peer to check";
    return;
  }

  if (_connected_peers < _max_connections) {
    messages::Peer *peer;
    if (this->next_to_connect(&peer)) {
      LOG_DEBUG << this << " Asking to connect to " << *peer;
      peer->set_status(messages::Peer::CONNECTING);
      auto tmp_peer = std::make_shared<messages::Peer>();
      tmp_peer->CopyFrom(*peer);
      _tcp->connect(tmp_peer);
    } else {
      LOG_DEBUG << this << " No more REACHABLE peers";
    }
  }
}

std::shared_ptr<networking::Networking> Bot::networking() {
  return _networking;
}

std::shared_ptr<messages::Queue> Bot::queue() { return _queue; }

void Bot::subscribe(const messages::Type type,
                    messages::Subscriber::Callback callback) {
  _subscriber.subscribe(type, callback);
}

void Bot::join() { _networking->join(); }

Bot::~Bot() {
  _subscriber.unsubscribe();
  LOG_DEBUG << this << " From Bot destructor" << &_subscriber;
}

}  // namespace neuro
