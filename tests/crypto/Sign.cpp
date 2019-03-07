#include "crypto/Sign.hpp"
#include <gtest/gtest.h>
#include "messages/Address.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace crypto {
namespace tests {

TEST(Sign, transaction) {
  messages::Transaction transaction;
  transaction.mutable_id()->CopyFrom(messages::Hasher::random());
  auto input = transaction.add_inputs();
  auto output = transaction.add_outputs();
  input->mutable_id()->CopyFrom(messages::Hasher::random());
  input->set_output_id(0);
  input->set_signature_id(0);
  output->mutable_address()->CopyFrom(messages::Address::random());
  output->mutable_value()->CopyFrom(messages::NCCAmount(0));
  crypto::Ecc ecc;
  ASSERT_TRUE(sign({&ecc}, &transaction));
  ASSERT_EQ(transaction.signatures_size(), 1);
  const auto signature = transaction.signatures(0);
  ASSERT_TRUE(signature.has_key_pub());
  ASSERT_TRUE(signature.has_signature());
  ASSERT_TRUE(verify(transaction));
  auto mutable_signature =
      transaction.mutable_signatures(0)->mutable_signature();
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
