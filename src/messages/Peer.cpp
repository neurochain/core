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

Peer::Peer(const std::string &endpoint, const Port port,
           const crypto::KeyPub &ecc_pub) {
  set_endpoint(endpoint);
  set_port(port);
  ecc_pub.save(mutable_key_pub());
  if (!has_status()) {
    set_status(Peer::DISCONNECTED);
  }
}

void Peer::set_status(::neuro::messages::_Peer_Status value) {
  _Peer::set_status(value);
  update_timestamp();
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
  if (next_update().data() < t &&
      status() & (Peer::UNREACHABLE | Peer::CONNECTING)) {
    this->set_status(Peer::DISCONNECTED);
  }
}

}  // namespace messages

}  // namespace neuro
