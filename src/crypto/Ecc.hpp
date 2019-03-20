#ifndef NEURO_SRC_CRYPTO_ECC_HPP
#define NEURO_SRC_CRYPTO_ECC_HPP

#include <memory>

#include "common/types.hpp"
#include "crypto/KeyPriv.hpp"
#include "crypto/KeyPub.hpp"

namespace neuro {
namespace crypto {

namespace test {
class Ecc;
}  // namespace test

class Ecc {
 public:
 private:
  std::shared_ptr<CryptoPP::AutoSeededRandomPool> _prng;
  KeyPriv _key_private;
  KeyPub _key_public;

 public:
  Ecc();
  Ecc(const std::string &filepath_private, const std::string &filepath_public);

  Ecc(const KeyPriv &ecc_priv, const KeyPub &ecc_pub);

  const KeyPriv &private_key() const;
  const KeyPub &public_key() const;

  KeyPriv *mutable_private_key();
  KeyPub *mutable_public_key();

  bool operator==(const Ecc &ecc) const;

  bool save(const std::string &filepath_private,
            const std::string &filepath_public) const;
  Buffer sign(const Buffer &input);
  Buffer sign(const uint8_t *data, const std::size_t size);
  void sign(const uint8_t *data, const std::size_t size, uint8_t *dest);
  static constexpr std::size_t sign_length() { return KeyPriv::sign_length(); }
  friend class test::Ecc;
};

}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_ECC_HPP */
