#ifndef NEURO_SRC_MESSAGES_PEER_HPP
#define NEURO_SRC_MESSAGES_PEER_HPP

#include "common.pb.h"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "crypto/KeyPub.hpp"

namespace neuro {
namespace messages {

namespace test {
class PeersF;
}  // namespace test

class Peer : public _Peer {
 private:
 public:
  Peer();

  Peer(const _Peer &peer);
  Peer(const std::string &endpoint, const Port port,
       const crypto::KeyPub &ecc_pub);

  void set_status(::neuro::messages::_Peer_Status value);
  void update_timestamp(std::time_t tick = 7);
  void update_unreachable(const std::time_t t);

  friend class neuro::messages::test::PeersF;
};

}  // namespace messages

}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_PEER_HPP */
