#include "Bot.hpp"
#include <cpr/cpr.h>
#include <algorithm>

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
    : _queue(),
      _config(config),
      _networking(&_queue, &_peers, _config.mutable_networking()),
      _subscriber(&_queue),
      _io_context(std::make_shared<boost::asio::io_context>()),
      _update_timer(*_io_context) {
  if (!init()) {
    throw std::runtime_error("Could not create bot from configuration file");
  }
}

Bot::Bot(const std::string &config_path)
    : Bot::Bot(messages::config::Config(config_path)) {}

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
  _networking.send_unicast(message);
}

void Bot::handler_block(const messages::Header &header,
                        const messages::Body &body) {
  //bool reply_message = header.has_request_id();
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
  _networking.send(message);

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
  _networking.send(message);

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
  _max_connections = networking_conf->max_connections();

  if (networking_conf->has_peers_update_time()) {
    _update_time = networking_conf->peers_update_time();
  }

  if (!networking_conf->has_tcp()) {
    LOG_ERROR << "Missing tcp configuration";
    return false;
  }

  auto tcpconfig = networking_conf->mutable_tcp();

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

  _io_context_thread = std::thread([this]() { _io_context->run(); });

  if (!_config.has_rest()) {
    LOG_INFO << "Missing rest configuration, not loading module";
  } else {
    const auto rest_config = _config.rest();
    _rest = std::make_shared<rest::Rest>(this, _ledger, _keys,
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
  auto msg = std::make_shared<messages::Message>();
  messages::fill_header(msg->mutable_header());
  msg->add_bodies()->mutable_get_peers();

  _networking.send(msg);
}

void Bot::update_connection_graph() {
  if (!_config.has_connection_graph_uri()) {
    return;
  }
  std::string uri = _config.connection_graph_uri();
  messages::ConnectionsGraph graph;
  messages::Address own_address = messages::Hasher(_keys->public_key());
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
  }

  _networking.send_unicast(msg);
}

void Bot::handler_peers(const messages::Header &header,
                        const messages::Body &body) {
  LOG_DEBUG << this << " Got a Peers message";
  add_peers(body.peers().peers());
}

void Bot::handler_connection(const messages::Header &header,
                             const messages::Body &body) {
  LOG_DEBUG << this << " It entered in handler_connection in bot " << body;

  auto connection_ready = body.connection_ready();

  if (connection_ready.from_remote()) {
    // Nothing to do; just wait for the hello message from remote peer
    // add the peer from the header to my list of peers
    //_tcp_config->add_peers()->CopyFrom(peer);
    // add_peer(peer);
    // Nothing else to do; just wait for the hello message from remote peer
    return;
  }

  // send hello msg
  auto message = std::make_shared<messages::Message>();
  messages::fill_header(message->mutable_header());

  auto hello = message->add_bodies()->mutable_hello();
  hello->set_listen_port(_networking.listening_port());

  auto key_pub = hello->mutable_key_pub();
  key_pub->set_type(messages::KeyType::ECP256K1);
  const auto tmp = _keys->public_key().save();
  key_pub->set_raw_data(tmp.data(), tmp.size());

  _networking.send_unicast(message);
  LOG_DEBUG << this << __LINE__
            << " _networking.peer_count(): " << _networking.peer_count();
}

void Bot::handler_deconnection(const messages::Header &header,
                               const messages::Body &body) {
  if (header.has_connection_id()) {
    _networking.terminate(header.has_connection_id());
  }

  LOG_DEBUG << this << " " << __LINE__
            << " _networking.peer_count(): " << _networking.peer_count();

  this->keep_max_connections();
}

void Bot::handler_world(const messages::Header &header,
                        const messages::Body &body) {
  auto world = body.world();
  add_peers(world.peers());

  if (!world.accepted()) {
    LOG_DEBUG << this << " Not accepted, disconnecting ...";
    _networking.terminate(header.connection_id());
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
  bool accepted = _networking.peer_count() < (2 * _max_connections);

  messages::Peer *remote_peer;
  if (hello.has_listen_port() && hello.has_endpoint()) {
    remote_peer = _peers.insert(hello.key_pub(),
				std::make_optional(hello.endpoint()),
				std::make_optional(hello.listen_port()));
  } else {
    remote_peer = _peers.insert(hello.key_pub(), {}, {});
  }
  
  remote_peer->set_status(messages::Peer::CONNECTED);
  if(hello.has_listen_port()) {
    remote_peer->set_port(hello.listen_port());
  }

  // update peer status
  if (accepted) {
    remote_peer->set_status(messages::Peer::CONNECTED);
  } else {
    remote_peer->set_status(messages::Peer::DISCONNECTED);
  }

  auto header_reply = message->mutable_header();
  messages::fill_header_reply(header, header_reply);
  world->set_accepted(accepted);

  Buffer key_pub_buffer;
  _keys->public_key().save(&key_pub_buffer);
  auto key_pub = world->mutable_key_pub();
  key_pub->set_type(messages::KeyType::ECP256K1);
  key_pub->set_hex_data(key_pub_buffer.str());

  if (!_networking.send_unicast(message)) {
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

std::ostream &operator<<(std::ostream &os, const neuro::Bot &bot) {

  // This is *NOT* json
  os << "Bot(" << &bot << ") { connected: ";
  auto used_peers = bot.peers().used_peers();
  std::copy(used_peers.begin(), used_peers.end(),
	    std::ostream_iterator<messages::Peer>(os, " "));
    
  os << " }";
  return os;
}

// Bot::Status Bot::status() const {
//   // std::lock_guard<std::mutex> lock_connections(_mutex_connections);
//   auto peers_size = _tcp_config->peers().size();
//   auto status =
//       Bot::Status(_networking.peer_count(), _max_connections, peers_size);
//   return status;
// }

std::vector<messages::Peer>Bot::connected_peers() const {
  return _peers.connected_peers();
}

  
bool Bot::next_to_connect(messages::Peer **peer) {
  // it is locked from the caller
  auto peers = _tcp_config->mutable_peers();

  // for (auto &peer : *_tcp_config->mutable_peers()) {
  //   LOG_TRACE << this << " PEER STATUS " << peer.endpoint() << ":"
  //             << peer.port() << peer.Status_Name(peer.status()) << std::endl;
  // }

  // Create a vector with all possible positions shuffled
  std::vector<std::size_t> pos((std::size_t)peers->size());
  std::iota(pos.begin(), pos.end(), 0);
  std::srand(unsigned(std::time(0)));
  std::random_shuffle(pos.begin(), pos.end());

  // Check every pos until we find one that is good to use
  for (const auto &idx : pos) {
    auto tmp_peer = _tcp_config->peers(idx);
    LOG_DEBUG << this << " wtf " << tmp_peer;
    // auto &tmp_peer = peers[idx];
    if (tmp_peer.status() != messages::Peer::CONNECTED) {
      *peer = &tmp_peer;
      return true;
    }
  }
  LOG_DEBUG << this << " could not find remote";
  return false;
}

void Bot::keep_max_connections() {
  if (const auto peer_count = _peers.used_peers_count();
      peer_count >= _max_connections) {
    LOG_INFO << this << " Already connected to " << peer_count << " / "
             << _max_connections;
    return;
  }

  auto peer = _peers.find_random(messages::Peer::DISCONNECTED, 2s);
  if (!peer) {
    LOG_WARNING << "Could not find peer to connect to";
    return;
  }

  LOG_DEBUG << this << " Asking to connect to " << *peer;
  peer->set_status(messages::Peer::CONNECTING);
  _networking.connect(peer);
}

networking::Networking *Bot::networking() { return &_networking; }

messages::Queue *Bot::queue() { return &_queue; }
  const messages::Peers &Bot::peers() const { return _peers; }
void Bot::subscribe(const messages::Type type,
                    messages::Subscriber::Callback callback) {
  _subscriber.subscribe(type, callback);
}

void Bot::publish_transaction(const messages::Transaction &transaction) const {
  // Add the transaction to the transaction pool

  // Send the transaction on the network
  auto message = std::make_shared<messages::Message>();
  messages::fill_header(message->mutable_header());
  auto body = message->add_bodies();
  body->mutable_transaction()->CopyFrom(transaction);
  _networking.send(message);
}

void Bot::join() { _networking.join(); }

Bot::~Bot() {
  _update_timer.cancel();
  _io_context->stop();

  while (!_io_context->stopped()) {
    LOG_INFO << this << " waiting ...";
    std::this_thread::sleep_for(10ms);
  }
  _subscriber.unsubscribe();
  if (_io_context_thread.joinable()) {
    _io_context_thread.join();
  }
  LOG_DEBUG << this << " From Bot destructor " << &_subscriber;
}

}  // namespace neuro
