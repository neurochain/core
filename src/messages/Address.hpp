#ifndef NEURO_SRC_MESSAGES_ADDRESS_HPP
#define NEURO_SRC_MESSAGES_ADDRESS_HPP

#include "messages/Hasher.hpp"
#include "common/logger.hpp"
#include "common.pb.h"

namespace neuro {
namespace messages {

class Address: public _Address {
private:
  int hash_size = 10;
  int checksum_size = 2;

  void load(const messages::KeyPub &key_pub) {
    auto key = crypto::hash_sha3_256(key_pub);
    auto checksum = crypto::hash_sha3_256(key);
    checksum.resize(checksum_size);
    key.append(checksum);
    set_data(key.data(), key.size());
  }
  
public:
  Address(const messages::KeyPub &key_pub) {
    load(key_pub);
  }

  Address(const crypto::EccPub &ecc_pub) {
    messages::KeyPub key_pub;
    ecc_pub.save(&key_pub);
    load(key_pub);
  }
  
  Address(const Address &address) {}
  Address &operator=(const Address &) {}
  
  bool verify() const {
    if(!has_data()) {
      return false;
    }

    if(data().size() != (hash_size + checksum_size)) {
      return false;
    }
    
    const Buffer key(data().substr(0, hash_size));
    const Buffer checksum_from_key(data().substr(hash_size, checksum_size));
    auto checksum_computed = crypto::hash_sha3_256(key);
    
    if(checksum_computed != checksum_from_key) {
      LOG_INFO << "Key checksum error";
      return false;
    }

    return true;
  }

  operator bool () const {
    return verify();
  }
};

  
}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_ADDRESS_HPP */
