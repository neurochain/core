#ifndef NEURO_SRC_CRYPTO_KEYPRIV_HPP
#define NEURO_SRC_CRYPTO_KEYPRIV_HPP

#include <bits/stdint-uintn.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/integer.h>
#include <cstdint>
#include <memory>

#include "KeyPub.hpp"
#include "common.pb.h"
#include "common/Buffer.hpp"
#include "common/types.hpp"
#include "crypto/KeyPub.hpp"

namespace CryptoPP {
class ECP;
}  // namespace CryptoPP

namespace neuro {
namespace crypto {

class KeyPriv : public messages::_KeyPriv {
 private:
  std::shared_ptr<CryptoPP::AutoSeededRandomPool> _prng;
  CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP> _params;
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey _key;

 public:
  KeyPriv(std::shared_ptr<CryptoPP::AutoSeededRandomPool> prng);
  KeyPriv(std::shared_ptr<CryptoPP::AutoSeededRandomPool> prng,
          const std::string &filepath);
  KeyPriv(std::shared_ptr<CryptoPP::AutoSeededRandomPool> prng,
          const messages::_KeyPriv &key_priv);
  bool save(const std::string &filepath) const;
  bool save(Buffer *buffer) const;
  messages::_KeyPub save() const;
  bool save(messages::_KeyPriv *key_priv) const;
  bool load(const Buffer &buffer);
  bool load(const std::string &filepath);
  static constexpr std::size_t sign_length() { return 64; /* TODO  magic */ }
  void sign(const uint8_t *data, const std::size_t size,
            uint8_t *signature) const;
  Buffer sign(const uint8_t *data, const std::size_t size) const;
  Buffer sign(const Buffer &input) const;

  KeyPub make_key_pub() const;
  bool operator==(const KeyPriv &key) const;
  CryptoPP::Integer exponent() const;
  friend std::ostream &operator<<(std::ostream &os, const KeyPriv &priv);
};

std::ostream &operator<<(std::ostream &os, const KeyPriv &priv);

}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_ECCPRIV_HPP */
