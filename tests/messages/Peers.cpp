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
    ::neuro::_fake_time = true;

    keys0.key_pub().save(peer0.mutable_key_pub());
    keys1.key_pub().save(peer1.mutable_key_pub());
    keys2.key_pub().save(peer2.mutable_key_pub());

    peer0.set_status(Peer::CONNECTING);
    peer1.set_status(Peer::DISCONNECTED);
    peer2.set_status(Peer::CONNECTED);
  }
};

TEST_F(PeersF, insert_peer) {
  ::neuro::messages::Peers peers(peer0.key_pub());

  ASSERT_EQ(peers.size(), 0);
  peers.insert(peer0);
  ASSERT_EQ(peers.size(), 0);
  peers.insert(peer0);
  peers.insert(peer1);
  ASSERT_EQ(peers.size(), 1);

  crypto::Ecc new_keys;

  new_keys.key_pub().save(peer0.mutable_key_pub());

  ASSERT_TRUE(peers.insert(peer0));
  ASSERT_EQ(peers.size(), 2);
}

TEST_F(PeersF, pick) {
  ::neuro::messages::Peers peers(peer0.key_pub());

  peers.insert(peer0);
  auto new_peer1 = peers.insert(peer1);
  auto new_peer2 = peers.insert(peer2);
  peers.insert(peer3);

  ASSERT_EQ(peers.size(), 3) << peers;

  (*new_peer1)->set_status(Peer::CONNECTED);
  (*new_peer2)->set_status(Peer::CONNECTING);

  ASSERT_EQ(peers.used_peers_count(), 1);
  const auto used_peers = peers.used_peers();
  ASSERT_EQ(used_peers.size(), 2);

  const auto disconnected_peers = peers.by_status(Peer::DISCONNECTED);
  ASSERT_EQ(disconnected_peers.size(), 1);
}

TEST_F(PeersF, update_unreachable) {
  ::neuro::messages::Peers peers(peer0.key_pub());

  peers.insert(peer0);
  auto new_peer1 = peers.insert(peer1);
  auto new_peer2 = peers.insert(peer2);
  auto new_peer3 = peers.insert(peer3);

  ASSERT_EQ(peers.size(), 3) << peers;

  (*new_peer1)->set_status(Peer::UNREACHABLE);
  (*new_peer2)->set_status(Peer::CONNECTING);
  (*new_peer3)->set_status(Peer::CONNECTED);

  ASSERT_EQ(peers.by_status(Peer::DISCONNECTED).size(), 0);
  ASSERT_EQ(peers.by_status(Peer::UNREACHABLE).size(), 1);
  ASSERT_EQ(peers.by_status(Peer::CONNECTING).size(), 1);
  ::neuro::time(10u);

  ASSERT_EQ(peers.by_status(Peer::DISCONNECTED).size(), 1);
  ASSERT_EQ(peers.by_status(Peer::UNREACHABLE).size(), 1);
  ASSERT_EQ(peers.by_status(Peer::CONNECTING).size(), 0);
}

TEST_F(PeersF, iterator) {
  ::neuro::messages::Peers peers(peer0.key_pub());

  peer1.set_status(Peer::CONNECTED);
  peers.insert(peer0);
  peers.insert(peer1);
  peers.insert(peer2);
  peers.insert(peer3);

  int peer_cout = 0;
  for (auto it = peers.begin(Peer::DISCONNECTED), e = peers.end(); it != e;
       ++it) {
    peer_cout++;
  }
  ASSERT_EQ(peers.size(), 3);

  // TODO: operator*
}

}  // namespace test
}  // namespace messages
}  // namespace neuro
