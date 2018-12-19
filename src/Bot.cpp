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
#include "consensus/PiiConsensus.hpp"
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

  messages::Message message;
  auto header_reply = message.mutable_header();
  auto id = messages::fill_header_reply(header, header_reply);

  if (get_block.has_hash()) {
    const auto previd = get_block.hash();
    auto block = message.add_bodies()->mutable_block();
    if (!_ledger->get_block_by_previd(previd, block)) {
      std::stringstream sstr;  // TODO operator <<
      sstr << previd;
      LOG_ERROR << this << " get_block by prev id not found " << sstr.str();
      return;
    }

  } else if (get_block.has_height()) {
    const auto height = get_block.height();
    for (auto i = 0u; i < get_block.count(); ++i) {
      auto block = message.add_bodies()->mutable_block();
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
  networking::Networking::RemoteKey key;  // todo: assign properly
  _networking->send_unicast(key, message);
}

void Bot::handler_block(const messages::Header &header,
                        const messages::Body &body) {
  LOG_TRACE;
  bool reply_message = header.has_request_id();
  _consensus->add_block(body.block(), !reply_message);
  update_ledger();

  if (header.has_request_id()) {
    auto got = _request_ids.find(header.request_id());
    if (got != _request_ids.end()) {
      return;
    }
  }

  messages::Message message;
  auto header_reply = message.mutable_header();
  auto id = messages::fill_header(header_reply);
  message.add_bodies()->mutable_block()->CopyFrom(body.block());
  _networking->send(message);

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

  messages::Message message;
  auto header = message.mutable_header();
  auto idheader = messages::fill_header(header);

  const auto id = last_header.id();

  LOG_DEBUG << "Search Block -" << id;

  auto get_block = message.add_bodies()->mutable_get_block();
  get_block->mutable_hash()->CopyFrom(id);
  get_block->set_count(1);
  _networking->send(message);

  _request_ids.insert(idheader);
  return false;
}

void Bot::subscribe() {

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
  _max_connections = networking_conf->max_connections();

  if (networking_conf->has_peers_update_time()) {
    _update_time = networking_conf->peers_update_time();
  }

  if (!networking_conf->has_tcp()) {
    LOG_ERROR << "Missing tcp configuration";
    return false;
  }

  auto tcpconfig = networking_conf->mutable_tcp();
  _peer_pool = std::make_shared<networking::PeerPool>(tcpconfig->peers());

  auto port = tcpconfig->port();
  _networking->create_tcp(_keys, port, _max_connections);
  return true;
}

bool Bot::init() {
  if (_config.has_logs()) {
    log::from_config(_config.logs());
  }

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

  _consensus = std::make_shared<consensus::PiiConsensus>(_io_context, _ledger,
                                                         _networking);
  _consensus->add_wallet_keys(_keys);
  _io_context_thread = std::thread([this]() { _io_context->run(); });

  if (!_config.has_rest()) {
    LOG_INFO << "Missing rest configuration, not loading module";
  } else {
    const auto rest_config = _config.rest();
    _rest = std::make_shared<rest::Rest>(this, _ledger, _keys, _consensus,
                                         rest_config);
  }

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
  LOG_DEBUG << this << peer_list_str();
  messages::Message msg;
  messages::fill_header(msg.mutable_header());
  msg.add_bodies()->mutable_get_peers();

  _networking->send(msg);
}

std::set<networking::PeerPool::PeerPtr> Bot::connected_peers() const {
  if (!_peer_pool) {
    return {};
  }
  return _networking->connected_peers(*_peer_pool);
}

std::size_t Bot::nb_connected_peers() const {
  return _networking->peer_count();
}

void Bot::update_connection_graph() {
  assert(!!_peer_pool);
  if (!_config.has_connection_graph_uri()) {
    return;
  }
  std::string uri = _config.connection_graph_uri();
  messages::ConnectionsGraph graph;
  messages::Address own_address = messages::Hasher(_keys->public_key());
  graph.mutable_own_address()->CopyFrom(own_address);
  auto peers_connected = connected_peers();
  for (const auto &peer : peers_connected) {
    if (!peer->has_key_pub()) {
      LOG_ERROR << "Missing key on peer " << *peer;
      continue;
    }
    crypto::EccPub ecc_pub;
    ecc_pub.load(peer->key_pub());
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
  messages::Message msg;
  auto header_reply = msg.mutable_header();
  messages::fill_header_reply(header, header_reply);
  auto key_pub = header_reply->mutable_key_pub();
  key_pub->set_type(messages::KeyType::ECP256K1);
  const auto remote_key = _keys->public_key().save();
  key_pub->set_raw_data(remote_key.data(), remote_key.size());
  auto peers_body = msg.add_bodies()->mutable_peers();
  assert(!!_peer_pool);
  auto peers = _peer_pool->peers_message();
  auto &iterable_peers = peers.peers();
  for (const auto &peer_conn : iterable_peers) {
    auto tmp_peer = peers_body->add_peers();
    tmp_peer->mutable_key_pub()->CopyFrom(peer_conn.key_pub());
    tmp_peer->set_endpoint(peer_conn.endpoint());
    tmp_peer->set_port(peer_conn.port());
  }
  _networking->send_unicast(remote_key, msg);
}

void Bot::handler_peers(const messages::Header &header,
                        const messages::Body &body) {
  LOG_DEBUG << this << " Got a Peers message";
  _peer_pool->insert(body.peers());
}

void Bot::handler_connection(const messages::Header &header,
                             const messages::Body &body) {
  LOG_DEBUG << this << " It entered in handler_connection in bot " << body;
  const auto type = get_type(body);
  if (type != messages::Type::kConnectionReady) {
    LOG_ERROR << this << " Unexpected message type.";
    return;
  }
  // TODO: do something usefull...
}

void Bot::handler_deconnection(const messages::Header &header,
                               const messages::Body &body) {
  LOG_DEBUG << this << " It entered in handler_deconnection in bot " << body;
  const auto type = get_type(body);
  if (type != messages::Type::kConnectionClosed) {
    LOG_ERROR << this << " Unexpected message type.";
    return;
  }
  // TODO: do something usefull...
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
  // auto peers_size = _peer_pool.size();
  // auto status = Bot::Status(_networking.connected_peers(), _max_connections,
  // peers_size);
  auto status = Bot::Status(0,_max_connections, 0);
  return status;
}

void Bot::keep_max_connections() {
  LOG_TRACE;
  LOG_INFO << "Entered keep_max_connections";
  auto peers = connected_peers();
  for (const auto &peer : peers) {
    LOG_INFO << "Connected peer " << *peer;
  }

  std::size_t current_peer_count = nb_connected_peers();
  LOG_INFO << "Number of peers with status connected: " << current_peer_count
           << std::endl
           << *this;
  assert(!!_peer_pool);
  _networking->keep_max_connections(*_peer_pool);
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
  messages::Message message;
  messages::fill_header(message.mutable_header());
  auto body = message.add_bodies();
  body->mutable_transaction()->CopyFrom(transaction);
  _networking->send(message);
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

std::string Bot::peer_list_str() const {
  std::stringstream ss;
  auto peers = _peer_pool->get_peers();
  for (const auto& peer: peers) {
    ss << *peer << std::endl;
  }
  return ss.str();
}

}  // namespace neuro
