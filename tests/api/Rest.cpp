#include "api/Rest.hpp"
#include <gtest/gtest.h>
#include <src/crypto/Hash.hpp>

namespace neuro::api::test {

TEST(Rest, to_internal_id) {
  messages::TransactionID test_transaction_from_user;
  messages::Transaction test_transaction;
  // base64 & base58 of the same sha
  messages::from_json(R"({"type":"SHA256", "data":"A8tR7VrNBp9ZdnEPbLHeoGkxLnsBAZQWgqjtVk7Ddbas"})", &test_transaction_from_user);
  messages::from_json(R"({"id":{"type":"SHA256","data":"h75iSkWlN53Z78ejSHEy4wx5qV4glWnNwqK8q+ICBFQ="},"inputs":[{"id":{"type":"SHA256","data":"iN0DNP+1hOlA3CB2c15BUHfC8OVnFd4MDvfPPhZg78g="},"outputId":0,"signatureId":0},{"id":{"type":"SHA256","data":"tDfNuNuzlkuy3FYcVoCvDLMmifTeXEPbgah1xHHjHU0="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"1"}},{"address":{"data":"N5U2TQWMWfXF5z5bBVpnT4KzxpkYuFGnho"},"value":{"value":"2"}}],"signatures":[{"signature":{"type":"SHA256","data":"hOGo2sNhLVYgUmVLXGzUbaCvaP5xrqX9OmNpKmTsUC9T51MpA+mbf6XvkxfKD9euCMj7gvkYqFUH/k4+vs8sAw=="},"keyPub":{"type":"ECP256K1","rawData":"AgNEbgj8H4laaNQlIwH0rXiG+KQWEjDV5cBHCMkQ5X+i"}},{"signature":{"type":"SHA256","data":"NWGRS4rLxCf2sXCKFzMS7jt7nzH1DWy7PwVr+Jcn3dBFZCdRiE+9shSj5lXwgmzjnTqrhllGBjR0KAtsWrDJ+w=="},"keyPub":{"type":"ECP256K1","rawData":"AgNEbgj8H4laaNQlIwH0rXiG+KQWEjDV5cBHCMkQ5X+i"}}]})", &test_transaction);

  // transaction are hashed with empty data
  test_transaction.mutable_id()->set_data("");
  Buffer hash = crypto::hash_sha3_256(test_transaction);
  std::string internal_id = to_internal_id(test_transaction_from_user.data());

  ASSERT_EQ(hash.str(), internal_id);
}

}  // namespace neuro::api::test
