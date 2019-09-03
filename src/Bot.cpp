#include "Bot.hpp"
#include "api/Rest.hpp"
#include "common/logger.hpp"
#include "messages/Subscriber.hpp"

namespace neuro {
using namespace std::chrono_literals;

Bot::Bot(const messages::config::Config &config,
         const consensus::Config &consensus_config)
    : _config(config),
      _io_context(std::make_shared<boost::asio::io_context>()),
      _queue(),
      _subscriber(&_queue),
      _keys(_config.networking()),
      _me(_config.networking(), _keys.at(0).key_pub()),
      _peers(_me.key_pub(), _config.networking()),
      _networking(&_queue, &_keys.at(0), &_peers, _config.mutable_networking()),
      _ledger(std::make_shared<ledger::LedgerMongodb>(_config.database())),
      _update_timer(std::ref(*_io_context)),
      _consensus_config(consensus_config) {
  if (!init()) {
    throw std::runtime_error("Could not create bot from configuration file");
  }

  LOG_DEBUG << this << " Bot started " << _me.port() << std::endl
            << _keys.at(0).key_pub() << std::endl
            << "cons " << _consensus.get() << std::endl
            << "net  " << &_networking << std::endl
            << "peers " << &_peers << std::endl
            << _peers << std::endl;
}

Bot::Bot(const std::string &config_path)
    : Bot::Bot(messages::config::Config(config_path)) {}

void Bot::handler_get_block(const messages::Header &header,
                            const messages::Body &body) {
  LOG_DEBUG << this << " : " << _me.port() << " Got a get_block message";
  const auto &get_block = body.get_block();

  auto message = std::make_shared<messages::Message>();
  auto header_reply = message->mutable_header();
  auto id = messages::fill_header_reply(header, header_reply);

  if (get_block.has_hash()) {
    const auto &id = get_block.hash();
    auto block = message->add_bodies()->mutable_block();
    if (!_ledger->get_block(id, block)) {
      LOG_ERROR << _me.port() << " get_block not found for id " << id;
      return;
    }

  } else if (get_block.has_height()) {
    const auto height = get_block.height();
    for (auto i = 0u; i < get_block.count(); ++i) {
      auto block = message->add_bodies()->mutable_block();
      if (!_ledger->get_block(height + i, block)) {
        LOG_ERROR << this << " : " << _me.port()
                  << " get_block by height not found";
        return;
      }
    }
  } else {
    LOG_ERROR << this << " : " << _me.port() << " get_block message ill-formed";
    return;
  }

  _request_ids.insert(id);
  _networking.send_unicast(message);
}

void Bot::handler_block(const messages::Header &header,
                        const messages::Body &body) {
  // bool reply_message = header.has_request_id();
  LOG_TRACE;
  if (!_consensus->add_block(body.block())) {
    LOG_WARNING << "Consensus rejected block";
    return;
  }
  update_ledger(_ledger->new_missing_block(body.block()));

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
  _consensus->add_transaction(body.transaction());
}

bool Bot::update_ledger(const std::optional<messages::Hash> &missing_block) {
  if (!missing_block) {
    return false;
  }

  auto message = std::make_shared<messages::Message>();
  auto header = message->mutable_header();
  auto idheader = messages::fill_header(header);

  auto get_block = message->add_bodies()->mutable_get_block();
  get_block->mutable_hash()->CopyFrom(*missing_block);
  get_block->set_count(1);
  _networking.send(message);

  _request_ids.insert(idheader);
  return false;
}

void Bot::update_ledger() {
  for (const auto &missing_block : _ledger->missing_blocks()) {
    update_ledger(missing_block);
  }
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
      _ledger, _keys, _consensus_config,
      [this](const messages::Block &block) { publish_block(block); });

  _io_context_thread = std::thread([this]() { _io_context->run(); });

  while (_io_context->stopped()) {
    std::this_thread::sleep_for(1s);
  }

  if (_config.has_rest()) {
    _api = std::make_unique<api::Rest>(_config.rest(), this);
    LOG_INFO << "api launched on : " << _config.rest().port();
  }

  this->keep_max_connections();

  return true;
}

void Bot::regular_update() {
  _peers.update_unreachable();
  update_peerlist();
  keep_max_connections();
  update_ledger();
  _networking.clean_old_connections(
      _config.networking().keep_old_connection_time());

  if (_config.has_random_transaction() &&
      rand() < _config.random_transaction() * static_cast<float>(RAND_MAX)) {
    send_random_transaction();
  }
  _update_timer.expires_at(_update_timer.expiry() +
                           boost::asio::chrono::seconds(_update_time));
  _update_timer.async_wait(boost::bind(&Bot::regular_update, this));
}

void Bot::send_random_transaction() {
  const auto peers = _peers.connected_peers();
  if (peers.size() == 0) {
    return;
  }
  const auto recipient = peers[rand() % peers.size()];
  const auto transaction = _ledger->send_ncc(
      _keys[0].key_priv(), messages::_KeyPub(recipient->key_pub()), 0.5);
  if (_consensus->add_transaction(transaction)) {
    LOG_DEBUG << this << " : " << _me.port() << " Sending random transaction "
              << transaction;
    publish_transaction(transaction);
  } else {
    LOG_WARNING << this << " : " << _me.port()
                << " Random transaction is not valid " << transaction;
  }
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
//   messages::_KeyPub own_key_pub = messages::_KeyPub(_keys.public_key());
//   graph.mutable_own_key_pub()->CopyFrom(own_key_pub);
//   messages::Peers peers;
//   for (const auto &peer : _tcp_config->peers()) {
//     if (!peer.has_key_pub()) {
//       LOG_ERROR << "Missing key on peer " << peer;
//       continue;
//     }
//     crypto::EccPub ecc_pub;
//     ecc_pub.load_from_point(peer.key_pub());
//     graph.add_peers_key_pubs()->CopyFrom(messages::_KeyPub(ecc_pub));
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

  for (const auto peer_conn : peers().peers_copy()) {
    auto tmp_peer = peers_body->add_peers();
    tmp_peer->mutable_key_pub()->CopyFrom(peer_conn.key_pub());
    tmp_peer->set_endpoint(peer_conn.endpoint());
    tmp_peer->set_port(peer_conn.port());
  }
  LOG_DEBUG << this << " : " << _me.port()
            << " got a get_peers message : " << peers_body;
  _networking.send_unicast(msg);
}

void Bot::handler_peers(const messages::Header &header,
                        const messages::Body &body) {
  const auto &peers = body.peers().peers();
  LOG_DEBUG << this << " : " << _me.port()
            << " got a peers message, receiving : " << peers;
  for (const auto &remote_peer : peers) {
    messages::Peer peer(_config.networking(), remote_peer);
    _peers.upsert(peer);
  }
}

void Bot::handler_connection(const messages::Header &header,
                             const messages::Body &body) {
  auto &connection_ready = body.connection_ready();
  if (connection_ready.from_remote()) {
    // ignore connection message; wait for an hello
    return;
  }

  // successfully established tcp connection; send hello msg
  auto message = std::make_shared<messages::Message>();
  messages::fill_header_reply(header, message->mutable_header());
  auto hello = message->add_bodies()->mutable_hello();
  hello->mutable_peer()->CopyFrom(_me);

  const auto tip = _ledger->get_main_branch_tip();
  hello->mutable_tip()->CopyFrom(tip.block().header().id());

  if (!_networking.send_unicast(message)) {
    LOG_DEBUG << this << " : " << _me.port() << " can't send hello message "
              << *message;
  }
  keep_max_connections();
}

void Bot::handler_deconnection(const messages::Header &header,
                               const messages::Body &body) {
  if (header.has_connection_id()) {
    auto remote_peer = _networking.find_peer(header.connection_id());
    if (remote_peer) {
      (*remote_peer)->set_status(messages::Peer::UNREACHABLE);
      LOG_DEBUG << this << " : " << _me.port() << " disconnected from "
                << (*remote_peer)->port();
    }
    _networking.terminate(header.connection_id());
  } else {
    // peer didn't create a connection
    if (body.connection_closed().has_peer()) {
      auto peer = _peers.find(body.connection_closed().peer().key_pub());
      if (peer) {
        (*peer)->set_status(messages::Peer::UNREACHABLE);
        LOG_DEBUG << _me.port() << " can't connect to " << (*peer)->port();
      }
    }
  }

  LOG_DEBUG << this << " : " << _me.port() << " " << __LINE__
            << " _networking.peer_count(): " << _networking.peer_count();

  this->keep_max_connections();
}

void Bot::handler_world(const messages::Header &header,
                        const messages::Body &body) {
  auto &world = body.world();
  auto remote_peer = _peers.find(header.key_pub());
  auto remote_peer_by_connection_id =
      _networking.find_peer(header.connection_id());

  assert(remote_peer);
  assert(remote_peer_by_connection_id);
  assert(*remote_peer == remote_peer_by_connection_id->get());
  if (!remote_peer) {
    LOG_WARNING << "Received world message from unknown peer "
                << header.key_pub();
    return;
  }
  if (!world.accepted()) {
    LOG_DEBUG << this << " : " << _me.port() << " Not accepted from "
              << (*remote_peer)->port() << ", disconnecting";
    _networking.terminate(header.connection_id());
    (*remote_peer)->set_status(messages::Peer::UNREACHABLE);
  } else {
    (*remote_peer)->set_status(messages::Peer::CONNECTED);
  }

  update_ledger(_ledger->new_missing_block(world));

  this->keep_max_connections();
}

void Bot::handler_hello(const messages::Header &header,
                        const messages::Body &body) {
  if (!body.has_hello()) {
    LOG_WARNING << this << " : " << _me.port()
                << " SomeThing wrong. Got a call to handler_hello with "
                   "different type of body on the msg";
    return;
  }
  const auto &hello = body.hello();
  const auto peer = _peers.find(header.key_pub());
  if (peer && ((*peer)->status() == messages::Peer::CONNECTED ||
               (*peer)->status() == messages::Peer::CONNECTING)) {
    LOG_WARNING << "Receiving an hello message from a peer we are already "
                   "connected to"
                << **peer;
    return;
  }
  const auto remote_peer_opt = _networking.find_peer(header.connection_id());
  messages::Peer *remote_peer;
  if (!remote_peer_opt) {
    LOG_WARNING << "Could not find peer we received message from";
    return;
  } else {
    const auto new_peer_opt = _peers.insert(*remote_peer_opt);
    if (!new_peer_opt) {
      LOG_WARNING << "Could not create peer";
      return;
    }
    remote_peer = *new_peer_opt;
  }
  LOG_DEBUG << "TOTORO0 " << *remote_peer;
  LOG_DEBUG << "TOTORO1 " << hello.peer();
  remote_peer->CopyFrom(hello.peer());

  LOG_DEBUG << this << " : " << _me.port() << " Got a HELLO message from "
            << remote_peer->port();

  // == Create world message for replying ==
  auto message = std::make_shared<messages::Message>();
  auto world = message->add_bodies()->mutable_world();
  auto peers = message->add_bodies()->mutable_peers();
  bool accepted = _peers.used_peers_count() < _max_incoming_connections;

  const auto tip = _ledger->get_main_branch_tip();
  world->mutable_missing_block()->CopyFrom(tip.block().header().id());

  // update peer status
  if (accepted) {
    remote_peer->set_status(messages::Peer::CONNECTED);
    LOG_DEBUG << this << " : " << _me.port() << " Accept status "
              << std::boolalpha << accepted << " " << *remote_peer << std::endl
              << _peers;
  } else {
    remote_peer->set_status(messages::Peer::DISCONNECTED);
  }

  auto header_reply = message->mutable_header();
  messages::fill_header_reply(header, header_reply);
  world->set_accepted(accepted);

  _peers.fill(peers);

  if (!_networking.send_unicast(message)) {
    LOG_ERROR << this << " : " << _me.port() << " Failed to send world message";
    remote_peer->set_status(messages::Peer::UNREACHABLE);
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
  const auto peer_count = _peers.used_peers_count();
  if (peer_count >= _max_connections) {
    LOG_INFO << this << " : " << _me.port() << " Already connected to "
             << peer_count << " / " << _max_connections << std::endl
             << _peers;
    return;
  }

  auto peer_it = _peers.begin(messages::Peer::DISCONNECTED);
  if (peer_it == _peers.end()) {
    LOG_WARNING << "Could not find peer to connect to";
    return;
  }

  LOG_DEBUG << this << " : " << _me.port() << " Asking to connect to "
            << **peer_it;
  (*peer_it)->set_status(messages::Peer::CONNECTING);
  if (!_networking.connect(*peer_it)) {
    (*peer_it)->set_status(messages::Peer::UNREACHABLE);
  }
}

const messages::Peers &Bot::peers() const { return _peers; }
void Bot::subscribe(const messages::Type type,
                    messages::Subscriber::Callback callback) {
  _subscriber.subscribe(type, callback);
}

bool Bot::publish_transaction(const messages::Transaction &transaction) const {
  // Add the transaction to the transaction pool
  _consensus->add_transaction(transaction);

  // Send the transaction on the network
  auto message = std::make_shared<messages::Message>();
  messages::fill_header(message->mutable_header());
  auto body = message->add_bodies();
  body->mutable_transaction()->CopyFrom(transaction);
  return (_networking.send(message) !=
          networking::TransportLayer::SendResult::FAILED);
}

void Bot::publish_block(const messages::Block &block) const {
  // Send the transaction on the network
  auto message = std::make_shared<messages::Message>();
  messages::fill_header(message->mutable_header());
  auto body = message->add_bodies();
  body->mutable_block()->CopyFrom(block);
  _networking.send(message);
}

ledger::Ledger *Bot::ledger() { return _ledger.get(); }

void Bot::join() { _networking.join(); }

Bot::~Bot() {
  _update_timer.cancel();
  _io_context->stop();

  while (!_io_context->stopped()) {
    LOG_INFO << this << " : " << _me.port() << " waiting ...";
    std::this_thread::sleep_for(10ms);
  }
  _subscriber.unsubscribe();
  if (_io_context_thread.joinable()) {
    _io_context_thread.join();
  }
  LOG_DEBUG << this << " : " << _me.port() << " From Bot destructor "
            << &_subscriber;
}

messages::Peers *global_evil_peers;
}  // namespace neuro
