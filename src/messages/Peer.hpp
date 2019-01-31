#ifndef NEURO_SRC_MESSAGES_PEER_HPP
#define NEURO_SRC_MESSAGES_PEER_HPP

#include <mutex>
#include "common.pb.h"
#include "common/types.hpp"
#include "crypto/EccPub.hpp"

namespace neuro {
namespace messages {

class Peer : public _Peer {
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
    mutable_last_update()->set_data(std::time(nullptr));
  }
};

}  // namespace messages

bool operator==(const messages::Peer &a, const messages::Peer &b);
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_PEER_HPP */
