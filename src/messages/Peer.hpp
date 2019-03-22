#ifndef NEURO_SRC_MESSAGES_PEER_HPP
#define NEURO_SRC_MESSAGES_PEER_HPP

#include "common.pb.h"
#include "common/types.hpp"
#include "common/logger.hpp"
#include "crypto/EccPub.hpp"

namespace neuro {
namespace messages {

namespace test {
class PeersF;
}  // namespace test

class Peer : public _Peer {
 private:
 public:
  Peer() { set_status(Peer::DISCONNECTED); }

  Peer(const _Peer &peer) {
    CopyFrom(peer);
    if (!has_status()) {
      set_status(Peer::DISCONNECTED);
    }
  }
  Peer(const std::string &endpoint, const Port port,
       const crypto::EccPub &ecc_pub) {
    set_endpoint(endpoint);
    set_port(port);
    ecc_pub.save(mutable_key_pub());
    if (!has_status()) {
      set_status(Peer::DISCONNECTED);
    }
  }

  void set_status(::neuro::messages::_Peer_Status value) {
    _Peer::set_status(value);
    update_timestamp();
  }

  void update_timestamp() {
    if (!_fake_time) {
      mutable_next_update()->set_data(std::time(nullptr) + 10);  // TODO: config
    } else {
      mutable_next_update()->set_data(::neuro::time() + 1);
    }
  }

  void update_unreachable(const std::time_t t) {
    if (next_update().data() < t &&
        status() & (Peer::UNREACHABLE | Peer::CONNECTING)) {
      this->set_status(Peer::DISCONNECTED);
    }
  }

  friend class neuro::messages::test::PeersF;
};

}  // namespace messages

}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_PEER_HPP */
