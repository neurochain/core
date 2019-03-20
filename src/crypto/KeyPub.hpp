#ifndef NEURO_SRC_CRYPTO_KEYPUB_HPP
#define NEURO_SRC_CRYPTO_KEYPUB_HPP

#include <eccrypto.h>
#include <files.h>
#include <filters.h>
#include <oids.h>
#include <osrng.h>
#include <memory>

#include "common/Buffer.hpp"

namespace neuro {
namespace crypto {

class KeyPub : public messages::_KeyPub {
 public:
  using Key = CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey;

 private:
  Key _key;
  bool load(const Buffer &buffer);
  bool load(const std::string &filepath);
  bool load(const uint8_t *data, const std::size_t size);
  bool load(const messages::_KeyPub &keypub);

 public:
  KeyPub(const KeyPub &key_pub) = default;
  KeyPub(const messages::_KeyPub &key_pub);
  KeyPub(const std::string &filepath);
  KeyPub(const Buffer &pub_key);
  KeyPub(const Key &key);

  Key *key();
  bool save(const std::string &filepath) const;
  bool save(Buffer *buffer) const;
  Buffer save() const;
  bool save(messages::_KeyPub *key_pub) const;
  bool save_as_hex(messages::_KeyPub *key_pub) const;
  bool verify(const Buffer &data, const uint8_t *signature,
              const std::size_t size) const;
  bool verify(const Buffer &data, const Buffer &signature) const;
  bool operator==(const KeyPub &key) const;
  friend std::ostream &operator<<(std::ostream &os, const KeyPub &k);
};  // namespace crypto

std::ostream &operator<<(std::ostream &os, const KeyPub &k);

}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_ECCPUB_HPP */
