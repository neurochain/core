#ifndef NEURO_SRC_CRYPTO_KEYPUB_HPP
#define NEURO_SRC_CRYPTO_KEYPUB_HPP

#include <cryptopp/eccrypto.h>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
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
  bool load(const Path &filepath);
  bool load(const uint8_t *data, const std::size_t size);
  bool load(const messages::_KeyPub &key_pub);

 public:
  KeyPub(const KeyPub &key_pub) = default;
  explicit KeyPub(const messages::_KeyPub &key_pub);
  explicit KeyPub(const Path &filepath);
  explicit KeyPub(const Buffer &key_pub);
  explicit KeyPub(const Key &key);
  KeyPub(const uint8_t *data, const std::size_t size);

  Key *key();
  bool save(const Path &filepath) const;
  bool save(Buffer *buffer) const;
  Buffer save() const;
  bool save(messages::_KeyPub *key_pub) const;
  bool save_as_hex(messages::_KeyPub *key_pub) const;
  bool verify(const Buffer &data, const uint8_t *signature,
              const std::size_t size) const;
  bool verify(const Buffer &data, const Buffer &signature) const;
  bool operator==(const KeyPub &key) const;
  //friend std::ostream &operator<<(std::ostream &os, const KeyPub &key_pub);
};  // namespace crypto

std::ostream &operator<<(std::ostream &os, const KeyPub &pub);

}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_ECCPUB_HPP */
