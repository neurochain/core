#include "Bot.hpp"
#include <cpr/cpr.h>
#include <algorithm>

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
    : _config(config),
      _io_context(std::make_shared<boost::asio::io_context>()),
      _queue(),
      _subscriber(&_queue),
      _keys(_config.networking().key_priv_path(),
            _config.networking().key_pub_path()),
      _me(_config.networking().tcp().endpoint(),
          _config.networking().tcp().port(), _keys.at(0).key_pub()),
      _peers(_me.key_pub(), _config.networking().tcp().peers().begin(),
             _config.networking().tcp().peers().end()),
      _networking(&_queue, &_keys.at(0), &_peers, _config.mutable_networking()),
      _ledger(std::make_shared<ledger::LedgerMongodb>(_config.database())),
      _update_timer(std::ref(*_io_context)) {
  LOG_DEBUG << this << " " << _me.port() << " hello from bot " << &_queue << " " << &_networking << " " 
            << _keys.at(0).key_pub() << std::endl
            << _peers << std::endl;

  if (!init()) {
    throw std::runtime_error("Could not create bot from configuration file");
  }
}

Bot::Bot(const std::string &config_path)
    : Bot::Bot(messages::config::Config(config_path)) {}

void Bot::handler_get_block(const messages::Header &header,
                            const messages::Body &body) {
  LOG_DEBUG << _me.port() << " Got a get_block message";
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
      LOG_ERROR << _me.port() << " get_block by prev id not found " << sstr.str();
      return;
    }

  } else if (get_block.has_height()) {
    const auto height = get_block.height();
    for (auto i = 0u; i < get_block.count(); ++i) {
      auto block = message->add_bodies()->mutable_block();
      if (!_ledger->get_block(height + i, block)) {
        LOG_ERROR << _me.port() << " get_block by height not found";
        return;
      }
    }
  } else {
    LOG_ERROR << _me.port() << " get_block message ill-formed";
    return;
  }

  _request_ids.insert(id);
  _networking.send_unicast(message);
}

void Bot::handler_block(const messages::Header &header,
                        const messages::Body &body) {
  // bool reply_message = header.has_request_id();
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
  if ((std::time(nullptr) - last_header.timestamp().data()) <
      _consensus->config().block_period) {
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

bool Bot::configure_networking(messages::config::Config *config) {
  const auto &networking_conf = config->networking();
  _max_connections = networking_conf.max_connections();
  _max_incoming_connections = 2 * _max_connections;

  _update_time = networking_conf.peers_update_time();

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

  configure_networking(&_config);
  _update_timer.expires_after(1s);
  _update_timer.async_wait(boost::bind(&Bot::regular_update, this));

  _consensus = std::make_shared<consensus::Consensus>(
      _ledger, _keys,
      [this](const messages::Block &block) { publish_block(block); });

  _io_context_thread = std::thread([this]() { _io_context->run(); });

  while (_io_context->stopped()) {
    std::this_thread::sleep_for(1s);
  }

  update_ledger();
  this->keep_max_connections();

  return true;
}

void Bot::regular_update() {
  _peers.update_unreachable();
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

// void Bot::update_connection_graph() {
//   if (!_config.has_connection_graph_uri()) {
//     return;
//   }
//   std::string uri = _config.connection_graph_uri();
//   messages::ConnectionsGraph graph;
//   messages::Address own_address = messages::Address(_keys.public_key());
//   graph.mutable_own_address()->CopyFrom(own_address);
//   messages::Peers peers;
//   for (const auto &peer : _tcp_config->peers()) {
//     if (!peer.has_key_pub()) {
//       LOG_ERROR << "Missing key on peer " << peer;
//       continue;
//     }
//     crypto::EccPub ecc_pub;
//     ecc_pub.load(peer.key_pub());
//     graph.add_peers_addresses()->CopyFrom(messages::Address(ecc_pub));
//   }

//   std::string json;
//   messages::to_json(graph, &json);
//   cpr::Post(cpr::Url{uri}, cpr::Body{json},
//             cpr::Header{{"Content-Type", "text/plain"}});
// }

void Bot::handler_get_peers(const messages::Header &header,
                            const messages::Body &body) {
  // build list of peers reachable && connected to send
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
  LOG_DEBUG << _me.port() << " got a get_peers message, sending : " << peers;
  _networking.send_unicast(msg);
}

void Bot::handler_peers(const messages::Header &header,
                        const messages::Body &body) {
  const auto &peers = body.peers().peers();
  LOG_DEBUG << _me.port() << " got a peers message, receiving : " << peers;
  for (const auto &peer : peers) {
    _peers.insert(peer);
  }
}

void Bot::handler_connection(const messages::Header &header,
                             const messages::Body &body) {
  auto connection_ready = body.connection_ready();
  if (connection_ready.from_remote()) {
  // Nothing to do; just wait for the hello message from remote peer
    return;
  }

  // send hello msg
  auto message = std::make_shared<messages::Message>();
  messages::fill_header_reply(header, message->mutable_header());

  auto hello = message->add_bodies()->mutable_hello();
  hello->mutable_peer()->CopyFrom(_me);

  if (!_networking.send_unicast(message)) {
    LOG_DEBUG << "can't respond to connection message " << _me;
  }
  keep_max_connections();
}

void Bot::handler_deconnection(const messages::Header &header,
                               const messages::Body &body) {
  if (header.has_connection_id()) {
    auto remote_peer = _networking.find_peer(header.connection_id());
    if (remote_peer) {
      (*remote_peer)->set_status(messages::Peer::UNREACHABLE);
      LOG_DEBUG << _me.port() << " disconnected from " << (*remote_peer)->port();
    }
    _networking.terminate(header.connection_id());
  }

  this->keep_max_connections();
}

void Bot::handler_world(const messages::Header &header,
                        const messages::Body &body) {
  auto world = body.world();
  auto remote_peer = _peers.find(header.key_pub());
  if (!remote_peer) {
    LOG_WARNING << "Received world message from unknown peer" << messages::to_json(body);
    return;
  }
  if (!world.accepted()) {
    LOG_DEBUG << _me.port() << " Not accepted, disconnecting ...";
    _networking.terminate(header.connection_id());
    (*remote_peer)->set_status(messages::Peer::UNREACHABLE);
  } else {
    (*remote_peer)->set_status(messages::Peer::CONNECTED);
  }
  this->keep_max_connections();
}

void Bot::handler_hello(const messages::Header &header,
                        const messages::Body &body) {
  if (!body.has_hello()) {
    LOG_WARNING << _me.port()
                << " SomeThing wrong. Got a call to handler_hello with "
                   "different type of body on the msg";
    return;
  }
  auto hello = body.hello();
  auto remote_peer = _peers.insert(hello.peer());

  if (!remote_peer) {
    LOG_WARNING << "Received a message from ourself (from the futuru?)";
    return;
  }

  // == Create world message for replying ==
  auto message = std::make_shared<messages::Message>();
  auto world = message->add_bodies()->mutable_world();
  auto peers = message->add_bodies()->mutable_peers();
  bool accepted = _peers.used_peers_count() < _max_incoming_connections;

  // update peer status
  if (accepted) {
    (*remote_peer)->set_status(messages::Peer::CONNECTED);
    LOG_DEBUG << _me.port() << " Accept status " << std::boolalpha << accepted << " "
              << **remote_peer << std::endl
              << _peers;
  } else {
    (*remote_peer)->set_status(messages::Peer::UNREACHABLE);
  }

  auto header_reply = message->mutable_header();
  messages::fill_header_reply(header, header_reply);
  world->set_accepted(accepted);

  _peers.fill(peers);

  if (!_networking.send_unicast(message)) {
    LOG_ERROR << _me.port() << " Failed to send world message: " << messages::to_json(*message);
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
  os << bot.peers();
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

std::vector<messages::Peer *> Bot::connected_peers() {
  return _peers.connected_peers();
}

void Bot::keep_max_connections() {
  if (const auto peer_count = _peers.used_peers_count();
      peer_count >= _max_connections) {
    LOG_INFO << _me.port() << " Already connected to " << peer_count << " / "
             << _max_connections << std::endl
             << _peers;
    return;
  }

  auto peer_it = _peers.begin(messages::Peer::DISCONNECTED);
  if (peer_it == _peers.end()) {
    LOG_WARNING << "Could not find peer to connect to";
    return;
  }

  LOG_DEBUG << _me.port() << " Asking to connect to " << **peer_it;
  (*peer_it)->set_status(messages::Peer::CONNECTING);
  _networking.connect(*peer_it);
}

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

void Bot::publish_block(const messages::Block &block) const {
  // Send the transaction on the network
  auto message = std::make_shared<messages::Message>();
  messages::fill_header(message->mutable_header());
  auto body = message->add_bodies();
  body->mutable_block()->CopyFrom(block);
  _networking.send(message);
}

void Bot::join() { _networking.join(); }

Bot::~Bot() {
  _update_timer.cancel();
  _io_context->stop();

  while (!_io_context->stopped()) {
    LOG_INFO << _me.port() << " waiting ...";
    std::this_thread::sleep_for(10ms);
  }
  _subscriber.unsubscribe();
  if (_io_context_thread.joinable()) {
    _io_context_thread.join();
  }
  LOG_DEBUG << _me.port() << " From Bot destructor " << &_subscriber;
}

}  // namespace neuro
