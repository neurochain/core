#include <gtest/gtest.h>
#include <typeinfo>

#include "crypto/Ecc.hpp"
#include "messages.pb.h"
#include "src/messages/Peers.hpp"

namespace neuro {
namespace messages {
namespace test {

class PeersF : public ::testing::Test {
 protected:
  Peer peer0;
  Peer peer1;
  Peer peer2;
  Peer peer3;

  crypto::Ecc keys0;
  crypto::Ecc keys1;
  crypto::Ecc keys2;

 public:
  PeersF() {
    Peer::_fake_time = true;
  
    keys0.public_key().save(peer0.mutable_key_pub());
    keys1.public_key().save(peer1.mutable_key_pub());
    keys2.public_key().save(peer2.mutable_key_pub());

    peer0.set_status(Peer::CONNECTING);
    peer1.set_status(Peer::DISCONNECTED);
    peer2.set_status(Peer::CONNECTED);
  }
};

TEST(PeersF, insert_peer) {
  Peer peer0;
  Peer peer1;

  ::neuro::messages::Peers peers;

  ASSERT_EQ(peers.size(), 0);
  peers.insert(peer0);
  ASSERT_EQ(peers.size(), 1);
  peers.insert(peer0);
  peers.insert(peer1);
  ASSERT_EQ(peers.size(), 1);

  crypto::Ecc keys0;
  crypto::Ecc keys1;

  keys0.public_key().save(peer0.mutable_key_pub());
  keys1.public_key().save(peer1.mutable_key_pub());

  ASSERT_EQ(peers.size(), 1);
  peers.insert(peer0);
  std::cout << peer0 << std::endl;
  std::cout << peers << std::endl;
  ASSERT_EQ(peers.size(), 2);
  peers.insert(peer0);
  peers.insert(peer1);
  ASSERT_EQ(peers.size(), 3);
}

TEST_F(PeersF, pick) {
  ::neuro::messages::Peers peers;

  peers.insert(peer0);
  peers.insert(peer1);
  peers.insert(peer2);
  peers.insert(peer3);

  ASSERT_EQ(peers.size(), 4);
  std::cout << peers << std::endl;

  ASSERT_EQ(peers.used_peers_count(), 2);
  const auto used_peers = peers.used_peers();
  ASSERT_EQ(used_peers.size(), 2);

  const auto disconnected_peers = peers.by_status(Peer::DISCONNECTED);
  ASSERT_EQ(disconnected_peers.size(), 2);
}

TEST_F(PeersF, update_timestamp) {
  ::neuro::messages::Peers peers;

  peer0.set_status(Peer::CONNECTING);
  peer1.set_status(Peer::DISCONNECTED);
  peer2.set_status(Peer::CONNECTED);

  peers.insert(peer0);
  peers.insert(peer1);
  peers.insert(peer2);
  peers.insert(peer3);

  ASSERT_EQ(peers.size(), 4);
  std::cout << peers << std::endl;

  ASSERT_EQ(peers.used_peers_count(), 2);
  const auto used_peers = peers.used_peers();
  ASSERT_EQ(used_peers.size(), 2);

  const auto disconnected_peers = peers.by_status(Peer::DISCONNECTED);
  ASSERT_EQ(disconnected_peers.size(), 2);
}

}  // namespace test
}  // namespace messages
}  // namespace neuro
