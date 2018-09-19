#ifndef NEURO_SRC_CRYPTO_ECCPUB_HPP
#define NEURO_SRC_CRYPTO_ECCPUB_HPP

#include <eccrypto.h>
#include <files.h>
#include <filters.h>
#include <oids.h>
#include <osrng.h>
#include <memory>

#include "common/Buffer.hpp"

namespace neuro {
namespace crypto {

class EccPub {
 public:
  using Key = CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PublicKey;

 private:
  Key _key;

 public:
  EccPub() {}
  EccPub(const EccPub &pub_key) = default;
  EccPub(const std::string &filepath);
  EccPub(const Buffer &pub_key);
  EccPub(const Key &key);
  
  Key *key();
  bool save(const std::string &filepath) const;
  bool load(const Buffer &buffer);
  bool load(const std::string &filepath);
  bool save(Buffer *buffer) const;
  bool verify(const Buffer &data, const uint8_t *signature,
              const std::size_t size) const;
  bool verify(const Buffer &data, const Buffer &signature) const;
  bool operator==(const EccPub &key) const;
  friend std::ostream &operator<<(std::ostream &os, const EccPub &k);
};  // namespace crypto

std::ostream &operator<<(std::ostream &os, const EccPub &k);

}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_ECCPUB_HPP */
