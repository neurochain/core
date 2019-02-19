#ifndef NEURO_SRC_MESSAGES_PEER_HPP
#define NEURO_SRC_MESSAGES_PEER_HPP

#include <mutex>
#include "common.pb.h"
#include "common/types.hpp"
#include "crypto/EccPub.hpp"

namespace neuro {
namespace messages {

namespace test {
class Peers;
}  // namespace test

class Peer : public _Peer {
 private:
  static bool _fake_time;

 public:
  Peer() {}
  Peer(const _Peer &peer) { CopyFrom(peer); }
  Peer(const std::string &endpoint, const Port port,
       const crypto::EccPub &ecc_pub) {
    set_endpoint(endpoint);
    set_port(port);
    ecc_pub.save(mutable_key_pub());
  }

  void set_status(::neuro::messages::_Peer_Status value) {
    _Peer::set_status(value);
    update_timestamp();
  }

  void update_timestamp() {
    if (!_fake_time) {
      mutable_last_update()->set_data(std::time(nullptr));
    } else {
      mutable_last_update()->set_data(::neuro::_time);
    }
  }

  friend class neuro::messages::test::Peers;
};

}  // namespace messages

}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_PEER_HPP */
