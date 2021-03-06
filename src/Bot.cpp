#include "Bot.hpp"
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include "api/Rest.hpp"
#include "api/GRPC.hpp"
#include "common/logger.hpp"
#include "messages/Subscriber.hpp"

namespace neuro {
using namespace std::chrono_literals;

Bot::Bot(const messages::config::Config &config,
         const consensus::Config &consensus_config)
    : _config(config), _io_context(std::make_shared<boost::asio::io_context>()),
      _queue(), _subscriber(&_queue), _keys(_config.networking()),
      _me(_config.networking(), _keys.at(0).key_pub()),
      _peers(_me.key_pub(), _config.networking()),
      _networking(&_queue, &_keys.at(0), &_peers, _config.mutable_networking()),
      _ledger(std::make_shared<ledger::LedgerMongodb>(_config.database())),
      _update_timer(*_io_context),
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
  const auto &get_block = body.get_block();
  LOG_DEBUG << this << " : " << _me.port() << " Got a get_block message ";

  auto message = std::make_shared<messages::Message>();
  auto header_reply = message->mutable_header();
  messages::fill_header_reply(header, header_reply);

  if (get_block.has_hash()) {
    const auto &id = get_block.hash();
    auto block = message->add_bodies()->mutable_block();
    if (!_ledger->get_block(id, block)) {
      LOG_DEBUG << _me.port() << " get_block not found for id " << id;
      message->mutable_bodies()->RemoveLast();
      auto *tip = message->add_bodies()->mutable_tip();

      auto tagged_block_tip = _ledger->get_main_branch_tip();
      tip->mutable_id()->CopyFrom(tagged_block_tip.block().header().id());
    }

  } else if (get_block.has_height()) {
    const auto height = get_block.height();
    for (auto i = 0u; i < get_block.count(); ++i) {
      auto block = message->add_bodies()->mutable_block();
      if (!_ledger->get_block(height + i, block)) {
        LOG_DEBUG << this << " : " << _me.port()
                  << " get_block by height not found";
        return;
      }
    }
  } else {
    LOG_ERROR << this << " : " << _me.port() << " get_block message ill-formed";
    return;
  }

  _networking.reply(message);
}

void Bot::handler_tip(const messages::Header &header,
                      const messages::Body &body) {

  const auto &tip = body.tip();
  if (tip.has_id()) {
    update_ledger(_ledger->new_missing_block(tip.id()));
  } else {
    update_ledger();
  }
}

void Bot::handler_block(const messages::Header &header,
                        const messages::Body &body) {
  messages::Message message;
  auto header_reply = message.mutable_header();
  messages::fill_header(header_reply);
  message.add_bodies()->mutable_block()->CopyFrom(body.block());
  if (!header.has_request_id()) {
    _networking.send_all(message);
  }

  if (!_consensus->add_block_async(body.block())) {
    LOG_WARNING << "Consensus rejected block" << body.block().header().id();
    return;
  }
  update_ledger(_ledger->new_missing_block(body.block().header().id()));

  if (header.has_request_id()) {
    auto got = _request_ids.find(header.request_id());
    if (got == _request_ids.end()) {
      LOG_WARNING << "The request_id is wrong " << body.block().header().id();
    }
  }
}

void Bot::handler_transaction(const messages::Header &header,
                              const messages::Body &body) {
  if (_consensus->add_transaction(body.transaction())) {
    messages::Message message;
    messages::fill_header(message.mutable_header());
    message.add_bodies()->mutable_transaction()->CopyFrom(body.transaction());
    _networking.send_all(message);
  }
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

  if (_networking.send_one(*message) ==
      networking::TransportLayer::SendResult::FAILED) {
    LOG_INFO << "no bot found to ask block " << *message;
  }

  _request_ids.insert(idheader);
  return false;
}

void Bot::update_ledger() {
  const auto missing_blocks = _ledger->missing_blocks();
  LOG_INFO << "Missing block count " << missing_blocks.size();
  for (const auto &missing_block : missing_blocks) {
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
      messages::Type::kTip,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_tip(header, body);
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
  _subscriber.subscribe(
      messages::Type::kHeartBeat,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_heart_beat(header, body);
      });
  _subscriber.subscribe(
      messages::Type::kPing,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handler_ping(header, body);
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
      [this](const messages::Block &block) { publish_block(block); },
      [this](const messages::Block &block) {
        auto publish_message = std::make_shared<messages::Message>();
        publish_message->add_bodies()
            ->mutable_publish()
            ->mutable_block()
            ->CopyFrom(block);
        _queue.push(publish_message);
      });

  _io_context_thread = std::thread([this]() { _io_context->run(); });

  while (_io_context->stopped()) {
    std::this_thread::sleep_for(1s);
  }

  if (_config.has_rest()) {
    _rest_api = std::make_unique<api::Rest>(_config.rest(), this);
    LOG_INFO << "rest api launched on : " << _config.rest().port();
  }

  if (_config.has_grpc()) {
    _grpc_api = std::make_unique<api::GRPC>(_config.grpc(), this);
    LOG_INFO << "grpc api launched on : " << _config.grpc().port();
  }

  return true;
}

void Bot::regular_update() {
  // don't add stuff here, use handler_heart_beat
  auto message = std::make_shared<messages::Message>();
  message->add_bodies()->mutable_heart_beat();
  _queue.push(message);
  _update_timer.expires_at(_update_timer.expiry() +
                           boost::asio::chrono::seconds(_update_time));
  _update_timer.async_wait(boost::bind(&Bot::regular_update, this));
}

void Bot::send_deferred() {
  for (auto &f : _deferred_world) {
    f();
  }
  _deferred_world.clear();
}

void Bot::handler_heart_beat(const messages::Header &header,
                             const messages::Body &body) {
  send_pings();
  _peers.update_unreachable();
  update_peerlist();
  keep_max_connections();
  update_ledger();
  _networking.clean_old_connections(
      _config.networking().keep_old_connection_time());

  if (_config.has_random_transaction() &&
      rand() < _config.random_transaction() * float(RAND_MAX)) {
    send_random_transaction();
  }
  send_deferred();
}

void Bot::handler_ping(const messages::Header &header,
                       const messages::Body &body) {
  auto remote_peer = _networking.find_peer(header.connection_id());
  if (remote_peer) {
    remote_peer->update_timestamp(
        _config.networking().connected_next_update_time());
  } else {
    LOG_WARNING << "Remote peer not found in handler_ping "
                << header.connection_id();
  }
}

void Bot::send_pings() {
  messages::Message message;
  messages::fill_header(message.mutable_header());
  message.add_bodies()->mutable_ping();
  _networking.send_all(message);
}

void Bot::send_random_transaction() {
  const auto peers = _peers.connected_peers();
  if (peers.size() == 0) {
    return;
  }
  const auto recipient = peers[rand() % peers.size()];
  messages::NCCAmount amount_to_send;
  if (_config.has_random_transaction_amount()) {
    amount_to_send.CopyFrom(_config.random_transaction_amount());
  } else {
    amount_to_send.CopyFrom(messages::NCCAmount(100));
  }

  const auto transaction = _ledger->send_ncc(
      _keys[0].key_priv(), messages::_KeyPub(recipient->key_pub()),
      amount_to_send);
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
  messages::Message msg;
  messages::fill_header(msg.mutable_header());
  msg.add_bodies()->mutable_get_peers();
  _networking.send_one(msg);
}

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
  _networking.reply(msg);
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
  keep_max_connections();
}

void Bot::handler_connection(const messages::Header &header,
                             const messages::Body &body) {
  auto &connection_ready = body.connection_ready();
  if (connection_ready.from_remote()) {
    // ignore connection message; wait for an hello
    keep_max_connections();
    return;
  }

  auto peer = _peers.find(header.key_pub());
  if (!peer) {
    LOG_WARNING << "Missing peer in handler_connection " << header.key_pub();
    keep_max_connections();
    return;
  }
  if (peer->status() == messages::Peer::CONNECTED) {
    // There is already a connection with this bot
    if (peer->has_connection_id() &&
        peer->connection_id() == header.connection_id()) {
      std::stringstream m;
      m << "You should not be here: entering handler_connection with a "
           "connected connection"
        << *peer;
      throw std::runtime_error(m.str());
    }
    _networking.terminate(header.connection_id());
    keep_max_connections();
    return;
  }

  // successfully established tcp connection; send hello msg
  auto message = std::make_shared<messages::Message>();
  messages::fill_header_reply(header, message->mutable_header());
  auto hello = message->add_bodies()->mutable_hello();
  hello->mutable_peer()->CopyFrom(_me);

  const auto tip = _ledger->get_main_branch_tip();
  hello->mutable_tip()->CopyFrom(tip.block().header().id());

  if (!_networking.reply(message)) {
    LOG_DEBUG << this << " : " << _me.port() << " can't send hello message "
              << *message;
  }
  keep_max_connections();
}

void Bot::handler_deconnection(const messages::Header &header,
                               const messages::Body &body) {
  LOG_DEBUG << _me.port() << " bot peers " << _peers;
  LOG_DEBUG << _me.port() << " networking peers " << _networking.pretty_peers()
            << std::endl;

  if (header.has_connection_id()) {
    auto remote_peer = _networking.find_peer(header.connection_id());
    if (remote_peer) {
      remote_peer->set_status(messages::Peer::UNREACHABLE);
      LOG_DEBUG << this << " : " << _me.port() << " disconnected from "
                << remote_peer->port();
    }
    _networking.terminate(header.connection_id());
  } else {
    // peer didn't create a connection
    if (body.connection_closed().has_peer()) {
      auto peer = _peers.find(body.connection_closed().peer().key_pub());
      if (peer) {
        peer->set_status(messages::Peer::UNREACHABLE);
        LOG_DEBUG << _me.port() << " can't connect to " << peer->port();
      }
    }
  }

  LOG_DEBUG << this << " : " << _me.port() << " " << __LINE__
            << " _networking.peer_count(): " << _networking.peer_count();

  keep_max_connections();
}

void Bot::handler_world(const messages::Header &header,
                        const messages::Body &body) {
  auto &world = body.world();
  auto remote_peer_bot = _peers.find(header.key_pub());
  auto remote_peer_connection = _networking.find_peer(header.connection_id());

  if (!remote_peer_connection) {
    LOG_WARNING << "Received world message but the connection is missing "
                << header.key_pub();
    _networking.terminate(header.connection_id());
    return;
  }
  if (!remote_peer_bot) {
    LOG_WARNING << "Received world message from unknown peer "
                << header.key_pub();
    _networking.terminate(header.connection_id());
    return;
  }

  assert(remote_peer_bot == remote_peer_connection);

  if (!world.accepted()) {
    LOG_DEBUG << this << " : " << _me.port() << " Not accepted from "
              << remote_peer_connection->port() << ", disconnecting";
    _networking.terminate(header.connection_id());
    remote_peer_connection->set_status(messages::Peer::UNREACHABLE);
  } else {
    remote_peer_connection->set_status(messages::Peer::CONNECTED);
  }

  update_ledger(_ledger->new_missing_block(world));

  keep_max_connections();
}

void Bot::handler_hello(const messages::Header &header,
                        const messages::Body &body) {
  auto remote_peer_connection = _networking.find_peer(header.connection_id());
  auto remote_peer_bot = _peers.find(header.key_pub());

  if (!remote_peer_connection) {
    LOG_WARNING << "Could not find peer we received message from";
    _networking.terminate(header.connection_id());
    return;
  }

  if (remote_peer_bot && remote_peer_bot != remote_peer_connection) {
    if (remote_peer_bot->status() == messages::Peer::CONNECTED) {
      LOG_DEBUG << _me.port()
                << " Refusing hello from a bot we are already connected to "
                << *remote_peer_bot;
      _networking.terminate(remote_peer_connection->connection_id());
      return;
    }
    if (remote_peer_bot->status() == messages::Peer::CONNECTING) {
      if (crypto::KeyPub(header.key_pub()) < _keys[0].key_pub()) {
        LOG_DEBUG << "Refusing hello from a bot we are connecting to "
                  << remote_peer_bot.get();
        _networking.terminate(remote_peer_connection->connection_id());
        return;
      } else {
        _networking.terminate(remote_peer_bot->connection_id());
      }
    }
  } else if (remote_peer_bot && remote_peer_bot == remote_peer_connection) {
    std::stringstream m;
    m << "You should not be here (handler_hello) " << *remote_peer_bot.get()
      << " " << remote_peer_connection;
    throw std::runtime_error(m.str());
  }

  _peers.insert(remote_peer_connection);
  if (remote_peer_bot && remote_peer_bot->connection_id() !=
                             remote_peer_connection->connection_id()) {
    LOG_WARNING
        << "new connection for the same bot accepted, closing the old one";
    _networking.terminate(remote_peer_bot->connection_id());
  }

  // == Create world message for replying ==
  auto message = std::make_shared<messages::Message>();
  auto world = message->add_bodies()->mutable_world();
  auto peers = message->add_bodies()->mutable_peers();
  const bool accepted = _peers.used_peers_count() < _max_incoming_connections;
  if (accepted) {
    remote_peer_connection->set_status(messages::Peer::CONNECTING);
  }
  const auto tip = _ledger->get_main_branch_tip();
  world->mutable_missing_block()->CopyFrom(tip.block().header().id());

  auto header_reply = message->mutable_header();
  messages::fill_header_reply(header, header_reply);
  world->set_accepted(accepted);

  _peers.fill(peers);

  _deferred_world.emplace_back([accepted, remote_peer_connection, this,
                                message]() {
    // update peer status
    if (accepted) {
      remote_peer_connection->set_status(messages::Peer::CONNECTED);
      LOG_DEBUG << _me.port() << " Accept status "
                << std::boolalpha << accepted << " " << *remote_peer_connection
                << std::endl
                << _peers;
    } else {
      remote_peer_connection->set_status(messages::Peer::UNREACHABLE);
    }

    if (!_networking.reply(message)) {
      LOG_ERROR << this << " : " << _me.port() << " Failed to send world message";
      remote_peer_connection->set_status(messages::Peer::UNREACHABLE);
    }
  });
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

std::vector<messages::Peer *> Bot::connected_peers() {
  return _peers.connected_peers();
}

void Bot::keep_max_connections() {
  LOG_DEBUG << _me.port() << " bot peers " << _peers;
  LOG_DEBUG << _me.port() << " networking peers " << _networking.pretty_peers()
            << std::endl;

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

const messages::Peer Bot::me() const { return _me; }

const messages::Peers &Bot::peers() const { return _peers; }

const messages::_Peers Bot::remote_peers() const {
  messages::_Peers peers;
  const auto remote_peers = _networking.remote_peers();
  for (const auto &remote_peer : remote_peers) {
    peers.add_peers()->CopyFrom(*remote_peer);
  }
  return peers;
}

void Bot::subscribe(const messages::Type type,
                    messages::Subscriber::Callback callback) {
  _subscriber.subscribe(type, callback);
}

bool Bot::publish_transaction(const messages::Transaction &transaction) const {
  // Add the transaction to the transaction pool
  _consensus->add_transaction(transaction);

  // Send the transaction on the network
  messages::Message message;
  messages::fill_header(message.mutable_header());
  auto body = message.add_bodies();
  body->mutable_transaction()->CopyFrom(transaction);
  return _networking.send_all(message) !=
         networking::TransportLayer::SendResult::FAILED;
}

void Bot::publish_block(const messages::Block &block) const {
  // Send the transaction on the network
  messages::Message message;
  messages::fill_header(message.mutable_header());
  auto body = message.add_bodies();
  body->mutable_block()->CopyFrom(block);
  _networking.send_all(message);

  LOG_INFO << "Publishing block " << block;
}

ledger::Ledger *Bot::ledger() { return _ledger.get(); }

void Bot::join() { _networking.join(); }

Bot::~Bot() {
  LOG_DEBUG << this << " : " << _me.port() << " entering bot destructor "
            << &_subscriber;

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
  LOG_DEBUG << this << " : " << _me.port() << " leaving bot destructor "
            << &_subscriber;
}

} // namespace neuro
