#include "messages/Peer.hpp"

namespace neuro {
namespace messages {

Peer::Peer() { set_status(Peer::DISCONNECTED); }

Peer::Peer(const _Peer &peer) {
  CopyFrom(peer);
  if (!has_status()) {
    set_status(Peer::DISCONNECTED);
  }
}

Peer::Peer(const std::string &endpoint,
           const Port port,
           const crypto::EccPub &ecc_pub) {
  set_endpoint(endpoint);
  set_port(port);
  ecc_pub.save(mutable_key_pub());
  if (!has_status()) {
    set_status(Peer::DISCONNECTED);
  }
}

void Peer::set_status(::neuro::messages::_Peer_Status value) {
  _Peer::set_status(value);
  switch(value) {
    case _Peer_Status_CONNECTING:
      update_timestamp(3);
      break;
    default:
      update_timestamp();  // default to 10 sec
      break;
  }
}

void Peer::update_timestamp(std::time_t tick) {
  int32_t next_tick;
  if (!_fake_time) {
    next_tick = static_cast<int32_t>(std::time(nullptr) + tick);
  } else {
    next_tick = static_cast<int32_t>(::neuro::time() + 1);
  }
  mutable_next_update()->set_data(next_tick);
}

void Peer::update_unreachable(const std::time_t t) {
  if (next_update().data() < t) {
    const auto peer_status = status();
    if (peer_status & Peer::UNREACHABLE) {
      this->set_status(Peer::DISCONNECTED);
    } else if (peer_status & Peer::CONNECTING) {
      this->set_status(Peer::UNREACHABLE);
    }
  }
}

}  // namespace messages

}  // namespace neuro
