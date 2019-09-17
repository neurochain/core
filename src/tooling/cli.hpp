#ifndef NEURO_SRC_TOOLING_CLI_HPP
#define NEURO_SRC_TOOLING_CLI_HPP

#include <Bot.hpp>
#include <future>

namespace neuro::tooling::cli {

std::string pretty_peer(const messages::_Peer &peer) {
  std::stringstream ss;
  ss << peer.endpoint() << " " << peer.key_pub() << " "
     << messages::_Peer_Status_Name(peer.status());
  return ss.str();
}

class BotCli : public Bot {
private:
  messages::Hash to_hash(const std::string &data) const {
    messages::Hash hash;
    messages::from_json("{data:\"" + data + "\"}", &hash);
    return hash;
  }

public:
  explicit BotCli(const messages::config::Config &c) : Bot(NoInit(), c) {
    _subscriber.subscribe(
        messages::Type::kHello,
        [&](const messages::Header &header, const messages::Body &body) {
          handler_hello(header, body);
        });

    _subscriber.subscribe(
        messages::Type::kWorld,
        [&](const messages::Header &header, const messages::Body &body) {
          handler_world(header, body);
        });

    _subscriber.subscribe(
        messages::Type::kConnectionClosed,
        [&](const messages::Header &header, const messages::Body &body) {
          handler_deconnection(header, body);
        });

    _subscriber.subscribe(
        messages::Type::kConnectionReady,
        [&](const messages::Header &header, const messages::Body &body) {
          handler_connection(header, body);
        });
  }

  void key(std::ostream &os) {
    os << _me << std::endl;
  }

  void get_block(std::ostream &os, const std::string &id) {
    messages::Message message;
    messages::fill_header(message.mutable_header());
    std::promise<int> promised_block;
    this->_subscriber.subscribe(message.header().id(), [&](const messages::Message &message){
      os << message << std::endl;
      promised_block.set_value(message.header().request_id());
    });
    auto *get_block = message.add_bodies()->mutable_get_block();
    get_block->mutable_hash()->CopyFrom(to_hash(id));
    get_block->set_count(1);
    send_one(message);
    promised_block.get_future().wait();
  }

  void connect(std::ostream &os) {
    auto peer_it = _peers.begin(messages::Peer::DISCONNECTED);
    if (peer_it == _peers.end()) {
      os << "can't found a bot to connect to" << std::endl;
      return;
    }
    auto peer = (*peer_it);
    peer->set_status(messages::Peer::CONNECTING);
    if (!_networking.connect(*peer_it)) {
      os << pretty_peer(*peer) << std::endl;
    }
  }
};
}  // namespace neuro::tooling
#endif // NEURO_SRC_TOOLING_CORE_CLI_HPP
