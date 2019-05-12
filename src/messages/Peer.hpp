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
  const ::neuro::messages::config::Networking &_config;

 public:
  Peer(const ::neuro::messages::config::Networking &config);

  Peer(const ::neuro::messages::config::Networking &config, const _Peer &peer);
  Peer(const ::neuro::messages::config::Networking &config,
       const crypto::KeyPub &ecc_pub);

  void set_status(::neuro::messages::_Peer_Status value);
  void update_timestamp(std::time_t tick);
  void update_unreachable(const std::time_t t);

  friend class neuro::messages::test::PeersF;
};

}  // namespace messages

}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_PEER_HPP */
