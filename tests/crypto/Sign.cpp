#include "crypto/Sign.hpp"
#include <gtest/gtest.h>
#include "messages/Address.hpp"
#include "messages/Hasher.hpp"

namespace neuro {
namespace crypto {
namespace tests {

TEST(Sign, transaction) {
  messages::Transaction transaction;
  transaction.mutable_id()->CopyFrom(messages::Hasher::random());
  transaction.mutable_last_seen_block_id()->CopyFrom(
      messages::Hasher::random());
  auto input = transaction.add_inputs();
  auto output = transaction.add_outputs();
  crypto::Ecc ecc0, ecc1;
  input->mutable_key_pub()->CopyFrom(ecc0.key_pub());
  input->mutable_value()->set_value(0);
  output->mutable_key_pub()->CopyFrom(ecc1.key_pub());
  output->mutable_value()->CopyFrom(messages::NCCAmount(0));
  ASSERT_TRUE(sign({&ecc0}, &transaction));
  ASSERT_TRUE(transaction.inputs(0).has_signature());
  ASSERT_TRUE(verify(transaction));
  auto mutable_signature = transaction.mutable_inputs(0)->mutable_signature();
  auto data = mutable_signature->mutable_data();
  (*data)[0] += 1;
  ASSERT_FALSE(verify(transaction));
}

TEST(Sign, block) {
  messages::Block block;
  auto header = block.mutable_header();
  header->mutable_id()->CopyFrom(messages::Hasher::random());
  header->mutable_timestamp()->set_data(0);
  header->mutable_previous_block_hash()->CopyFrom(messages::Hasher::random());
  header->set_height(0);
  crypto::Ecc keys;
  ASSERT_FALSE(header->has_author());
  ASSERT_TRUE(sign(keys, &block));
  ASSERT_TRUE(header->has_author());
  auto author = header->author();
  ASSERT_TRUE(author.has_key_pub());
  ASSERT_TRUE(author.has_signature());
  ASSERT_TRUE(verify(block));
  auto mutable_signature = header->mutable_author()->mutable_signature();
  auto data = mutable_signature->mutable_data();
  (*data)[0] += 1;
  ASSERT_FALSE(verify(block));
}

TEST(Sign, denunciation) {
  messages::Denunciation denunciation;
  denunciation.mutable_block_id()->CopyFrom(messages::Hasher::random());
  denunciation.set_block_height(0);
  crypto::Ecc keys;
  ASSERT_FALSE(denunciation.has_block_author());
  ASSERT_TRUE(sign(keys, &denunciation));
  ASSERT_TRUE(denunciation.has_block_author());
  auto author = denunciation.block_author();
  ASSERT_TRUE(author.has_key_pub());
  ASSERT_TRUE(author.has_signature());
  ASSERT_TRUE(verify(denunciation));
  auto mutable_signature =
      denunciation.mutable_block_author()->mutable_signature();
  auto data = mutable_signature->mutable_data();
  (*data)[0] += 1;
  ASSERT_FALSE(verify(denunciation));
}

}  // namespace tests
}  // namespace crypto
}  // namespace neuro
