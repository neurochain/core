#include "Bot.hpp"
#include <cpr/cpr.h>
#include <algorithm>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <random>
#include "common/logger.hpp"
#include "common/types.hpp"
#include "messages/Subscriber.hpp"

namespace neuro {
using namespace std::chrono_literals;

Bot::Bot(const messages::config::Config &config)
    : _queue(std::make_shared<messages::Queue>()),
      _networking(std::make_shared<networking::Networking>(_queue)),
      _subscriber(_queue),
      _io_context(std::make_shared<boost::asio::io_context>()),
      _config(config),
      _update_timer(*_io_context) {
  if (!init()) {
    throw std::runtime_error("Could not create bot from configuration file");
  }
}

Bot::Bot(const std::string &config_path)
    : Bot::Bot(messages::config::Config(config_path)) {}
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
  auto id = messages::fill_header_reply(header, header_reply);

  if (get_block.has_hash()) {
    const auto previd = get_block.hash();
    auto block = message->add_bodies()->mutable_block();
    if (!_ledger->get_block_by_previd(previd, block)) {
      std::stringstream sstr;  // TODO operator <<
      sstr << previd;
      LOG_ERROR << this << " get_block by prev id not found " << sstr.str();
      return;
    }

  } else if (get_block.has_height()) {
    const auto height = get_block.height();
    for (auto i = 0u; i < get_block.count(); ++i) {
      auto block = message->add_bodies()->mutable_block();
      if (!_ledger->get_block(height + i, block)) {
        LOG_ERROR << this << " get_block by height not found";
        return;
      }
    }
  } else {
    LOG_ERROR << this << " get_block message ill-formed";
    return;
  }

  _request_ids.insert(id);
  _networking->send_unicast(message, networking::ProtocolType::PROTOBUF2);
}

void Bot::handler_block(const messages::Header &header,
                        const messages::Body &body) {
  LOG_TRACE;
  _consensus->add_block(body.block());
  update_ledger();

  if (header.has_request_id()) {
    auto got = _request_ids.find(header.request_id());
    if (got != _request_ids.end()) {
      return;
    }
  }

  auto message = std::make_shared<messages::Message>();
  auto header_reply = message->mutable_header();
  auto id = messages::fill_header(header_reply);
  message->add_bodies()->mutable_block()->CopyFrom(body.block());
  _networking->send(message, networking::ProtocolType::PROTOBUF2);

  _request_ids.insert(id);
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

  // TODO #consensus change by height by time function
  if ((std::time(nullptr) - last_header.timestamp().data()) < BLOCK_PERIODE) {
    return true;
  }

  auto message = std::make_shared<messages::Message>();
  auto header = message->mutable_header();
  auto idheader = messages::fill_header(header);

  const auto id = last_header.id();

  LOG_DEBUG << "Search Block -" << id;

  auto get_block = message->add_bodies()->mutable_get_block();
  get_block->mutable_hash()->CopyFrom(id);
  get_block->set_count(1);
  _networking->send(message, networking::ProtocolType::PROTOBUF2);

  _request_ids.insert(idheader);
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

  _subscriber.subscribe(
      messages::Type::kGetPeers,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_get_peers(header, body);
      });

  _subscriber.subscribe(
      messages::Type::kPeers,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_peers(header, body);
      });
}

bool Bot::load_networking(messages::config::Config *config) {
  auto networking_conf = config->mutable_networking();
  _selection_method = config->selection_method();
  _keep_status = config->keep_status();
  _max_connections = networking_conf->max_connections();

  if (networking_conf->has_peers_update_time()) {
    _update_time = networking_conf->peers_update_time();
  }

  if (!networking_conf->has_tcp()) {
    LOG_ERROR << "Missing tcp configuration";
    return false;
  }

  auto tcpconfig = networking_conf->mutable_tcp();

  for (auto &peer : *tcpconfig->mutable_peers()) {
    peer.set_status(messages::Peer::REACHABLE);
    LOG_DEBUG << this << " Peer: " << peer;
  }

  auto port = tcpconfig->port();
  _tcp = _networking->create_tcp(_queue, _keys, port);
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

  _consensus = std::make_shared<consensus::Consensus>(_ledger);
  _consensus->add_wallet_key_pair(_keys);
  _io_context_thread = std::thread([this]() { _io_context->run(); });

  // if (!_config.has_rest()) {
  //   LOG_INFO << "Missing rest configuration, not loading module";
  // } else {
  //   const auto rest_config = _config.rest();
  //   _rest = std::make_shared<rest::Rest>(this, _ledger, _keys, rest_config);
  // }

  this->keep_max_connections();
  update_ledger();

  _update_timer.expires_after(boost::asio::chrono::seconds(_update_time));
  _update_timer.async_wait(boost::bind(&Bot::regular_update, this));

  LOG_DEBUG << this << " USING UPDATE TIME: " << _update_time;

  return true;
}

void Bot::regular_update() {
  update_peerlist();
  keep_max_connections();
  update_ledger();
  _update_timer.expires_at(_update_timer.expiry() +
                           boost::asio::chrono::seconds(_update_time));
  _update_timer.async_wait(boost::bind(&Bot::regular_update, this));
}

void Bot::update_peerlist() {
  LOG_DEBUG << this << " FROM UPDATE PEERLIST";
  for (auto &peer : *_tcp_config->mutable_peers()) {
    LOG_TRACE << this << " PEER STATUS " << peer.endpoint() << ":"
              << peer.port() << peer.Status_Name(peer.status()) << std::endl;
  }
  auto msg = std::make_shared<messages::Message>();
  messages::fill_header(msg->mutable_header());
  msg->add_bodies()->mutable_get_peers();

  _networking->send(msg, networking::ProtocolType::PROTOBUF2);
}

const std::vector<messages::Peer> Bot::connected_peers() const {
  std::vector<messages::Peer> res;
  for (const auto &peer : _tcp_config->peers()) {
    if (peer.status() == messages::Peer::CONNECTED) {
      res.push_back(peer);
    }
  }
  return res;
}

void Bot::update_connection_graph() {
  if (!_config.has_connection_graph_uri()) {
    return;
  }
  std::string uri = _config.connection_graph_uri();
  messages::ConnectionsGraph graph;
  const auto own_address = messages::Address{_keys->public_key()};
  graph.mutable_own_address()->CopyFrom(own_address);
  messages::Peers peers;
  for (const auto &peer : _tcp_config->peers()) {
    if (!peer.has_key_pub()) {
      LOG_ERROR << "Missing key on peer " << peer;
      continue;
    }
    crypto::EccPub ecc_pub;
    ecc_pub.load(peer.key_pub());
    graph.add_peers_addresses()->CopyFrom(messages::Hasher(ecc_pub));
  }

  std::string json;
  messages::to_json(graph, &json);
  cpr::Post(cpr::Url{uri}, cpr::Body{json},
            cpr::Header{{"Content-Type", "text/plain"}});
}

void Bot::handler_get_peers(const messages::Header &header,
                            const messages::Body &body) {
  LOG_DEBUG << this << " Got a Get_Peers message";
  // build list of peers reachabe && connected to send
  auto msg = std::make_shared<messages::Message>();
  auto header_reply = msg->mutable_header();
  messages::fill_header_reply(header, header_reply);
  auto peers_body = msg->add_bodies()->mutable_peers();

  auto peers = _tcp_config->mutable_peers();
  for (const auto &peer_conn : *peers) {
    auto tmp_peer = peers_body->add_peers();
    tmp_peer->mutable_key_pub()->CopyFrom(peer_conn.key_pub());
    tmp_peer->set_endpoint(peer_conn.endpoint());
    tmp_peer->set_port(peer_conn.port());
    tmp_peer->set_status(messages::Peer::REACHABLE);
  }

  _networking->send_unicast(msg, networking::ProtocolType::PROTOBUF2);
}

void Bot::handler_peers(const messages::Header &header,
                        const messages::Body &body) {
  LOG_DEBUG << this << " Got a Peers message";
  add_peers(body.peers().peers());
}

void Bot::handler_connection(const messages::Header &header,
                             const messages::Body &body) {
  LOG_DEBUG << this << " It entered in handler_connection in bot " << body;
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
    // add the peer from the header to my list of peers
    //_tcp_config->add_peers()->CopyFrom(peer);
    // add_peer(peer);
    // Nothing else to do; just wait for the hello message from remote peer
    return;
  }

  auto peers = _tcp_config->mutable_peers();
  auto it = std::find(peers->begin(), peers->end(), peer);
  if (it != peers->end()) {
    it->set_connection_id(peer.connection_id());
  }

  LOG_DEBUG << this << " Got a connection to " << peer;
  // send hello msg
  auto message = std::make_shared<messages::Message>();
  messages::fill_header(message->mutable_header());

  message->mutable_header()->mutable_peer()->CopyFrom(peer);
  auto hello = message->add_bodies()->mutable_hello();
  hello->set_listen_port(_tcp->listening_port());

  auto key_pub = hello->mutable_key_pub();
  key_pub->set_type(messages::KeyType::ECP256K1);
  const auto tmp = _keys->public_key().save();
  key_pub->set_raw_data(tmp.data(), tmp.size());

  _networking->send_unicast(message, networking::ProtocolType::PROTOBUF2);
  LOG_DEBUG << this << __LINE__
            << " _networking->peer_count(): " << _networking->peer_count();
}

void Bot::handler_deconnection(const messages::Header &header,
                               const messages::Body &body) {
  auto remote_peer = header.peer();
  LOG_DEBUG << this << " Got a connection_closed message "
            << remote_peer.status();
  // find the peer in our list of peers and update its status
  // std::lock_guard<std::mutex> lock_connections(_mutex_connections);
  auto peers = _tcp_config->mutable_peers();
  _tcp->terminate(remote_peer.connection_id());

  auto it = std::find(peers->begin(), peers->end(), remote_peer);
  if (it == peers->end()) {
    LOG_WARNING << "Unknown peer disconnected";
    this->keep_max_connections();

    return;
  }

  LOG_DEBUG << this << " " << __LINE__
            << " _networking->peer_count(): " << _networking->peer_count();
  it->set_status(messages::Peer::REACHABLE);

  this->keep_max_connections();
}

void Bot::handler_world(const messages::Header &header,
                        const messages::Body &body) {
  auto world = body.world();
  LOG_DEBUG << this << " Got a WORLD message";
  if (!header.has_peer()) {
    LOG_ERROR << this << " Got an empty peer. Going out of the world handler";
    return;
  }
  auto peers = _tcp_config->mutable_peers();
  add_peers(world.peers());

  auto peer_header = header.peer();

  // we should have it in our unconnected list because we told him "hello"
  auto peer_it = std::find(peers->begin(), peers->end(), peer_header);

  messages::Peer *remote_peer;
  if (peer_it == peers->end()) {
    auto remote_peer_opt = add_peer(peer_header);
    if (remote_peer_opt) {
      remote_peer = *remote_peer_opt;
    } else {
      LOG_ERROR << this << " Received a message from ourself ";
      return;
    }
    // remote_peer = _tcp_config->add_peers();
    // remote_peer->CopyFrom(peer_header);
    LOG_ERROR
        << "We should already have the peer if you receive a message from him";
  } else {
    remote_peer = &(*peer_it);
  }

  LOG_DEBUG << this << " The WORLD message is from " << *remote_peer;

  if (!world.accepted()) {
    LOG_DEBUG << this << " Not accepted, disconnecting ...";
    remote_peer->set_status(messages::Peer::FULL);
    // Should I call terminate from the connection
    _tcp->terminate(remote_peer->connection_id());
  } else {
    bool accepted = (_networking->peer_count() < _max_connections);

    if (!accepted) {
      LOG_DEBUG << this
                << " Closing a connection because remote is already full";
      remote_peer->set_status(messages::Peer::REACHABLE);
      _tcp->terminate(remote_peer->connection_id());
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

  LOG_DEBUG << this << " Got a HELLO message in bot";

  // == Create world message for replying ==
  auto message = std::make_shared<messages::Message>();
  auto world = message->add_bodies()->mutable_world();
  bool accepted = _networking->peer_count() < (2 * _max_connections);

  auto peers = _tcp_config->mutable_peers();
  auto peer_header = header.peer();
  peer_header.mutable_key_pub()->CopyFrom(hello.key_pub());

  // find the peer in my list of peer matching connection_id
  auto peer_it = std::find_if(
      peers->begin(), peers->end(), [&peer_header](const auto &it) {
        if (it.has_connection_id()) {
          return it.connection_id() == peer_header.connection_id();
        }
        return false;
      });
  bool found = peer_it != peers->end();
  LOG_DEBUG << this << " in handler_hello found: " << found;
  messages::Peer *remote_peer = nullptr;

  for (const auto &peer_conn : *peers) {
    if (!peer_conn.has_key_pub()) {
      continue;
    }
    auto tmp_peer = world->add_peers();
    tmp_peer->mutable_key_pub()->CopyFrom(peer_conn.key_pub());
    tmp_peer->set_endpoint(peer_conn.endpoint());
    tmp_peer->set_port(peer_conn.port());
    tmp_peer->set_status(messages::Peer::REACHABLE);
  }

  if (!found) {
    LOG_WARNING << this << " Peer was not created in handler_connection";
    auto remote_peer_opt = add_peer(peer_header);
    if (!remote_peer_opt) {
      LOG_ERROR << this << " Received a message from ourself ";
    } else {
      remote_peer = *remote_peer_opt;
    }
  } else {
    remote_peer = &(*peer_it);
  }

  remote_peer->set_status(messages::Peer::CONNECTED);

  // update port by listen_port
  if (remote_peer->has_connection_id()) {
    const auto port = _tcp->connection_port(remote_peer->connection_id());
    if (!!port) {
      remote_peer->set_port(*port);
    }
  } else {
    // TODO check this;
    LOG_ERROR << this << " The peer does not have a connection_id !!";
    return;
  }
  // update peer status
  if (accepted) {
    remote_peer->set_status(messages::Peer::CONNECTED);
  } else {
    remote_peer->set_status(messages::Peer::REACHABLE);
  }

  auto header_reply = message->mutable_header();
  messages::fill_header_reply(header, header_reply);
  world->set_accepted(accepted);

  Buffer key_pub_buffer;
  _keys->public_key().save(&key_pub_buffer);
  auto key_pub = world->mutable_key_pub();
  key_pub->set_type(messages::KeyType::ECP256K1);
  key_pub->set_hex_data(key_pub_buffer.str());

  if (!_networking->send_unicast(message,
                                 networking::ProtocolType::PROTOBUF2)) {
    LOG_ERROR << this << " Failed to send world message.";
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
  auto status =
      Bot::Status(_networking->peer_count(), _max_connections, peers_size);
  return status;
}

bool Bot::next_to_connect(messages::Peer **peer) {
  // it is locked from the caller
  auto peers = _tcp_config->mutable_peers();

  for (auto &peer : *_tcp_config->mutable_peers()) {
    LOG_TRACE << this << " PEER STATUS " << peer.endpoint() << ":"
              << peer.port() << peer.Status_Name(peer.status()) << std::endl;
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
  LOG_TRACE;
  LOG_INFO << "Entered keep_max_connections";
  auto peers = connected_peers();
  for (const auto &peer : peers) {
    LOG_INFO << "Connected peer " << peer;
  }
  LOG_INFO << "Number of peers with status connected: " << peers.size()
           << std::endl
           << *this;

  std::size_t peers_size = 0;
  peers_size = _tcp_config->peers().size();

  if (peers_size == 0) {
    LOG_INFO << this << " No peers";
    return;
  }
  LOG_DEBUG << this << " peer count " << peers_size;

  auto current_peer_count = _networking->peer_count();
  if (current_peer_count >= _max_connections) {
    LOG_INFO << this << " Already connected to " << current_peer_count << "/"
             << _max_connections;
    return;
  }

  if (current_peer_count == peers_size) {
    LOG_WARNING << this << " No available peer to check";
    return;
  }

  if (current_peer_count < _max_connections) {
    messages::Peer *peer;
    if (this->next_to_connect(&peer)) {
      LOG_DEBUG << this << " Asking to connect to " << *peer;
      peer->set_status(messages::Peer::CONNECTING);
      auto tmp_peer = std::make_shared<messages::Peer>();
      tmp_peer->CopyFrom(*peer);
      _tcp->connect(tmp_peer);
    } else {
      LOG_DEBUG << this << " No more REACHABLE peers - asking for peers";
      // this->update_peerlist();
    }
  } else {
    LOG_INFO << this << " Already connected to " << current_peer_count << "/"
             << _max_connections;
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

void Bot::publish_transaction(const messages::Transaction &transaction) const {
  // Add the transaction to the transaction pool
  _consensus->add_transaction(transaction);

  // Send the transaction on the network
  auto message = std::make_shared<messages::Message>();
  messages::fill_header(message->mutable_header());
  auto body = message->add_bodies();
  body->mutable_transaction()->CopyFrom(transaction);
  _networking->send(message, networking::ProtocolType::PROTOBUF2);
}

void Bot::join() { _networking->join(); }

Bot::~Bot() {
  _update_timer.cancel();
  _io_context->stop();

  while (!_io_context->stopped()) {
    LOG_INFO << this << " waiting ...";
    std::this_thread::sleep_for(10ms);
  }
  _subscriber.unsubscribe();
  _consensus.reset();
  if (_io_context_thread.joinable()) {
    _io_context_thread.join();
  }
  LOG_DEBUG << this << " From Bot destructor " << &_subscriber;
}

}  // namespace neuro
