#ifndef NEURO_SRC_CRYPTO_ECC_HPP
#define NEURO_SRC_CRYPTO_ECC_HPP

#include <memory>

#include "common/types.hpp"
#include "crypto/EccPriv.hpp"
#include "crypto/EccPub.hpp"

namespace neuro {
namespace crypto {

namespace test {
class Ecc;
}  // namespace test

class Ecc {
 public:
 private:
  std::shared_ptr<CryptoPP::AutoSeededRandomPool> _prng;
  std::unique_ptr<EccPriv> _key_private;
  std::unique_ptr<EccPub> _key_public;

  bool load_keys(const std::string &keypath_priv,
                 const std::string &keypath_pub);

 public:
  Ecc();
  Ecc(Ecc &&ecc) {}
  Ecc(const Ecc &) = delete;
  Ecc(const std::string &filepath_private, const std::string &filepath_public);

  Ecc(const EccPriv &ecc_priv, const EccPub &ecc_pub);

  const EccPriv &private_key() const;
  const EccPub &public_key() const;

  EccPriv *mutable_private_key();
  EccPub *mutable_public_key();

  bool operator==(const Ecc &ecc) const;

  bool save(const std::string &filepath_private,
            const std::string &filepath_public) const;
  Buffer sign(const Buffer &input);
  Buffer sign(const uint8_t *data, const std::size_t size);
  void sign(const uint8_t *data, const std::size_t size, uint8_t *dest);
  static constexpr std::size_t sign_length() { return EccPriv::sign_length(); }
  friend class test::Ecc;
};

}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_ECC_HPP */
