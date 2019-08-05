#include <bits/stdint-intn.h>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <cstdint>
#include <ctime>

#include "common/logger.hpp"
#include "common/types.hpp"
#include "messages/Peer.hpp"

namespace neuro {
namespace messages {

Peer::Peer(const ::neuro::messages::config::Networking &config)
    : _config(config) {
  set_status(Peer::DISCONNECTED);
}

Peer::Peer(const ::neuro::messages::config::Networking &config,
           const _Peer &peer) : _config(config) {
  CopyFrom(peer);
  if (!has_status()) {
    set_status(Peer::DISCONNECTED);
  }
}

Peer::Peer(const ::neuro::messages::config::Networking &config,
           const crypto::KeyPub &ecc_pub) : _config(config) {
  set_endpoint(config.tcp().endpoint());
  set_port(config.tcp().port());
  ecc_pub.save(mutable_key_pub());
  if (!has_status()) {
    set_status(Peer::DISCONNECTED);
  }
}

/**
 * change the status of a peer, and make it elligible for update
 * \param value a new status for the peer
 */
void Peer::set_status(::neuro::messages::_Peer_Status value) {
  _Peer::set_status(value);
  switch(value) {
    case _Peer_Status_CONNECTING:
      update_timestamp(_config.connecting_next_update_time());
      break;
    default:
      update_timestamp(_config.default_next_update_time());
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
      LOG_WARNING << this << " : " << port() << " connection timed out";
      this->set_status(Peer::UNREACHABLE);
    }
  }
}

}  // namespace messages

}  // namespace neuro
