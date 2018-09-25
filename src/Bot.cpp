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

// Bot::Bot(std::istream &bot_stream)
//     : Subscriber(std::make_shared<networking::Networking>()),
//       _conf(std::make_shared<config::Bot>(bot_stream)),
//       _peers_conf(std::make_shared<config::Peers>(peers_stream)) {
//   if (!init()) {
//     throw std::runtime_error("Could not create bot from configuration file");
//   }
// }

// Bot::Bot(const std::string &configuration_path)
//     : Subscriber(std::make_shared<networking::Networking>()),
//       _conf(std::make_shared<config::Bot>(configuration_path)) {
//   if (!init()) {
//     throw std::runtime_error("Could not create bot from configuration file");
//   }
// }

void Bot::handler_connection(const messages::Header &header,
                             const messages::Body &body) {
  // auto connection = static_cast<const messages::Connection *>(body.get());
  // auto connection_id = header->connection_id();
  // auto remote_peer = _tcp->remote_peer(connection_id);

  // if (!remote_peer) {
  //   LOG_ERROR << this
  //             << " Got an empty peer. Going out of the connection handler";
  //   return;
  // }

  // if (connection->from_remote) {
  //   // Nothing to do; just wait for the hello message from remote peer
  //   LOG_DEBUG << this << " Got a connection from " << *remote_peer;
  // } else {
  //   LOG_DEBUG << this << " Got a connection to " << *remote_peer;
  //   auto response_msg = std::make_shared<messages::Message>();
  //   response_msg->header()->connection_id(connection_id);
  //   response_msg->header()->transport_layer_id(header->transport_layer_id());
  //   response_msg->insert(
  //       std::make_shared<messages::Hello>(_tcp->listening_port()));
  //   _networking->send_unicast(response_msg);
  // }
}

void Bot::handler_deconnection(const messages::Header &header,
                               const messages::Body &body) {

  LOG_DEBUG << this << " Got a deconnection message";
  // auto deconnection = static_cast<const messages::Deconnection
  // *>(body.get()); auto remote_peer = deconnection->remote_peer; using ps =
  // networking::Peer::Status;

  // // If I was trying to connect then it is not reachable, else it means I was
  // // connected and it could be reachable
  // auto old_status = remote_peer->status();
  // if (old_status == ps::CONNECTING) {
  //   remote_peer->status(ps::UNREACHABLE);
  // } else if (old_status == ps::CONNECTED) {
  //   _connected_peers -= 1;
  // }

  // this->keep_max_connections();
}

void Bot::handler_world(const messages::Header &header,
                        const messages::Body &body) {
  LOG_DEBUG << this << " Got a WORLD message";
  // TODO: Validar aqui si ya alcance el max_connections
  // using ps = networking::Peer::Status;
  // auto world = static_cast<const messages::World *>(body.get());

  // if (world->peers.size() > 0) {
  //   LOG_DEBUG << this << " Peers list from remote bot: " << world->peers;
  // }

  // for (auto &peer : world->peers) {
  //   auto predicate = [peer, this](const auto &it) {
  //     return (it->ip() == peer->ip() && it->port() == peer->port()) ||
  //            (peer->ip() == this->_tcp->local_ip() &&
  //             peer->port() == this->_tcp->listening_port());
  //   };
  //   if (std::find_if(_peers.begin(), _peers.end(), predicate) !=
  //   _peers.end()) {
  //     continue;
  //   }

  //   _peers.emplace_back(peer);
  // }

  // auto connection_id = header->connection_id();
  // auto rpeer = _tcp->remote_peer(connection_id);

  // if (!rpeer) {
  //   LOG_ERROR << this << " Got an empty peer. Going out of the world
  //   handler"; return;
  // }

  // // we should have it in our unconnected list because we told him "hello"
  // auto peer_it =
  //     std::find_if(_peers.begin(), _peers.end(), [&rpeer](const auto &it) {
  //       return it->ip() == rpeer->ip() && it->port() == rpeer->port();
  //     });
  // std::shared_ptr<networking::Peer> remote_peer;
  // if (peer_it == _peers.end()) {
  //   remote_peer =
  //       std::make_shared<networking::Peer>(rpeer->ip(), rpeer->port());
  // } else {
  //   remote_peer = *peer_it;
  // }

  // LOG_DEBUG << this << " The WORLD message is from " << *remote_peer;

  // if (!world->accepted) {
  //   LOG_DEBUG << this << " Not accepted, disconnecting ...";
  //   remote_peer->status(ps::FULL);
  //   // Should I call terminate from the connection
  //   _tcp->disconnected(connection_id, remote_peer);
  // } else {
  //   bool accepted{false};
  //   {
  //     std::lock_guard<std::mutex> lock_connections(_mutex_connections);
  //     accepted = _connected_peers < _max_connections;
  //     if (accepted) {
  //       _connected_peers += 1;
  //     }
  //   }

  //   if (!accepted) {
  //     LOG_DEBUG << this << " Closing a connection because I am already full";
  //     remote_peer->status(ps::REACHABLE);
  //     _tcp->disconnected(connection_id, remote_peer);
  //   } else {
  //     remote_peer->status(ps::CONNECTED);
  //   }
  // }

  // this->keep_max_connections();
}

void Bot::handler_hello(const messages::Header &header,
                        const messages::Body &body) {
  LOG_DEBUG << this << " Got a HELLO message";
  // using ps = networking::Peer::Status;
  // auto hello = static_cast<const messages::Hello *>(body.get());
  // auto connection_id = header->connection_id();

  // // == Create world message for replying ==
  // networking::Peers peers_tosend;
  // auto message = std::make_shared<messages::Message>();
  // bool accepted, peers_connected{false};
  // {
  //   std::lock_guard<std::mutex> lock_connections(_mutex_connections);
  //   accepted = _connected_peers < _max_connections;
  //   peers_connected = _connected_peers > 0;
  //   if (accepted) {
  //     _connected_peers += 1;
  //   }
  // }

  // if (peers_connected) {
  //   for (const auto &peer_conn : _peers) {
  //     if (peer_conn->status() == ps::CONNECTED ||
  //         peer_conn->status() == ps::REACHABLE) {
  //       peers_tosend.emplace_back(peer_conn);
  //     }
  //   }
  // }

  // message->insert(
  //     std::make_shared<messages::World>(accepted, peers_tosend,
  //     header->id()));
  // message->header()->transport_layer_id(header->transport_layer_id());
  // message->header()->connection_id(connection_id);
  // _networking->send_unicast(message);

  // // == Check if we need to add this peer to our list or not ==
  // auto remote_ip = _tcp->remote_ip(connection_id);
  // if (remote_ip == networking::IP{}) {
  //   LOG_ERROR << this << " Got an empty IP. Going out of the hello handler";
  //   return;
  // }
  // auto peer_it = std::find_if(
  //     _peers.begin(), _peers.end(), [&hello, &remote_ip](const auto &it) {
  //       return it->ip() == remote_ip && it->port() == hello->listen_port;
  //     });
  // bool found = peer_it != _peers.end();

  // std::shared_ptr<networking::Peer> remote_peer;
  // if (!found) {
  //   remote_peer = std::make_shared<networking::Peer>(
  //       remote_ip, hello->listen_port, crypto::EccPub(header->sender_key()));
  // } else {
  //   remote_peer = *peer_it;
  // }

  // if (accepted) {
  //   remote_peer->status(ps::CONNECTED);
  // } else {
  //   remote_peer->status(ps::REACHABLE);
  // }

  // if (!found) {
  //   _peers.emplace_back(remote_peer);
  // }
}

bool Bot::init() {
  if (_config.has_logs()) {
    log::from_config(_config.logs());
  }
  const auto keypath_priv = _config.key_priv_path();
  const auto keypath_pub = _config.key_pub_path();
  bool keys_save{false};
  bool keys_create{false};
  LOG_DEBUG << "HELLO";
  std::cout << "WORLD" << std::endl;

  // if (keypath_pub.empty() && keypath_priv.empty()) {
  //   keys_create = true;
  // } else if (!boost::filesystem::exists(keypath_pub) ||
  //            !boost::filesystem::exists(keypath_priv)) {
  //   keys_create = true;
  //   keys_save = true;
  // }

  // if (!keys_create) {
  //   LOG_INFO << this << " Loading keys from " << keypath_priv << " and "
  //            << keypath_pub;
  //   _keys = std::make_shared<crypto::Ecc>(keypath_priv, keypath_pub);
  // } else {
  //   LOG_INFO << this << " Generating new keys";
  //   _keys = std::make_shared<crypto::Ecc>();
  //   if (keys_save) {
  //     LOG_INFO << this << " Saving keys to " << keypath_priv << " and "
  //              << keypath_pub;
  //     _keys->save(keypath_priv, keypath_pub);
  //   }
  // }

  // const auto &networking_conf = _conf->networking();
  // _selection_method = _conf->selection_method();
  // _max_connections = networking_conf.max_connections();
  // _keep_status = networking_conf.keep_status();
  // auto tcp_conf = networking_conf.tcp();
  // if (tcp_conf) {
  //   auto parser = messages::Parser::create(tcp_conf->parser());
  //   _tcp = std::make_shared<networking::Tcp>(_networking->message_queue(),
  //                                            _keys, parser);

  //   for (const auto port : tcp_conf->listen_ports()) {
  //     _tcp->accept(port);
  //     LOG_INFO << this << " Accepting connections on port " << port;
  //   }

  //   _networking->push(_tcp);
  //   auto peers_filepath = tcp_conf->peers_filepath();
  //   if (!peers_filepath.empty()) {
  //     _peers_conf = std::make_shared<config::Peers>(peers_filepath);
  //   }

  //   if (_peers_conf) {
  //     _peers = _peers_conf->networking_peers();
  //   } else {
  //     LOG_WARNING << this << " There is no information about peers";
  //   }
  // }

  // _subscriber->subscribe(messages::Type::kHello,
  //                        std::bind(&Bot::handler_hello, this,
  //                                  std::placeholders::_1,
  //                                  std::placeholders::_2));

  messages::Subscriber::Callback cb =
      [this](const messages::Header &header,
             const messages::Body &body) {
        this->handler_hello(header, body);
      };

  _subscriber->subscribe(messages::Type::kHello, cb);
  // _networking->subscribe(this, messages::Type::WORLD,
  //                        [this](std::shared_ptr<const messages::Header>
  //                        header,
  //                               std::shared_ptr<const messages::Body> body) {
  //                          this->handler_world(header, body);
  //                        });
  // _networking->subscribe(this, messages::Type::CONNECTION,
  //                        [this](std::shared_ptr<const messages::Header>
  //                        header,
  //                               std::shared_ptr<const messages::Body> body) {
  //                          this->handler_connection(header, body);
  //                        });
  // _networking->subscribe(this, messages::Type::DECONNECTION,
  //                        [this](std::shared_ptr<const messages::Header>
  //                        header,
  //                               std::shared_ptr<const messages::Body> body) {
  //                          this->handler_deconnection(header, body);
  //                        });
  return true;
}

void Bot::update_peers_file(const std::string &filename) const {
  // Writing only the connected peers
  // networking::Peers to_write;
  // using ps = networking::Peer::Status;
  // for (const auto &peer : _peers) {
  //   if (_keep_status == ps::FULL || peer->status() == _keep_status) {
  //     to_write.emplace_back(peer);
  //   }
  // }
  // _peers_conf->write(to_write);
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
  // auto status = Bot::Status(_connected_peers, _max_connections,
  // _peers.size()); return status;
}

bool Bot::next_to_connect(std::shared_ptr<networking::Peer> &peer) {
  // using ps = networking::Peer::Status;
  // using sm = config::Bot::SelectionMethod;
  // switch (_selection_method) {
  // case sm::SIMPLE: {
  //   //
  //   for (const auto &tmp_peer : _peers) {
  //     if (tmp_peer->status() == ps::REACHABLE) {
  //       peer = tmp_peer;
  //       return true;
  //     }
  //   }
  // } break;
  // case sm::PING: {
  //   LOG_WARNING << this
  //               << " SelectionMethod::PING is not implemented - Using
  //               RANDOM";
  // } // break; // TODO: After implementing PING, remove the comment from break
  // case sm::RANDOM: {
  //   // Create a vector with all possible positions shuffled
  //   std::vector<std::size_t> pos(_peers.size());
  //   std::iota(pos.begin(), pos.end(), 0);
  //   std::srand(unsigned(std::time(0)));
  //   std::random_shuffle(pos.begin(), pos.end());

  //   // Check every pos until we find one that is good to use
  //   for (const auto &idx : pos) {
  //     auto &tmp_peer = _peers[idx];
  //     if (tmp_peer->status() == ps::REACHABLE) {
  //       peer = tmp_peer;
  //       return true;
  //     }
  //   }
  // } break;
  // default:
  //   LOG_ERROR << this << " Uknown method for selecting next peer to connect
  //   to";
  // }
  // return false;
}

void Bot::keep_max_connections() {
  // if (_peers.empty()) {
  //   LOG_INFO << this << " Stayng server mode";
  //   return;
  // }
  // {
  //   std::lock_guard<std::mutex> lock_connections(_mutex_connections);
  //   if (_connected_peers == _max_connections)
  //     return;

  //   if (_connected_peers == _peers.size()) {
  //     LOG_WARNING << this << " There is no available peer to check";
  //     return;
  //   }

  //   if (_connected_peers < _max_connections) {
  //     std::shared_ptr<networking::Peer> peer;
  //     if (this->next_to_connect(peer)) {
  //       LOG_DEBUG << this << " Asking to connect to " << *peer;
  //       peer->status(networking::Peer::Status::CONNECTING);
  //       _tcp->connect(peer);
  //     } else {
  //       LOG_DEBUG << this << " No more REACHABLE peers";
  //     }
  //   }
  // }
}

void Bot::join() {
  this->keep_max_connections();
}

Bot::~Bot() {}

} // namespace neuro
