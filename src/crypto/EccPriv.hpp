#ifndef NEURO_SRC_CRYPTO_ECCPRIV_HPP
#define NEURO_SRC_CRYPTO_ECCPRIV_HPP

#include <sha3.h>
#include <memory>
#include <filters.h>

#include "EccPub.hpp"
#include "common/types.hpp"

namespace neuro {
namespace crypto {

class EccPriv {
 private:
  std::shared_ptr<CryptoPP::AutoSeededRandomPool> _prng;
  CryptoPP::DL_GroupParameters_EC<CryptoPP::ECP> _params;
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey _key;

 public:
  EccPriv(std::shared_ptr<CryptoPP::AutoSeededRandomPool> prng);
  EccPriv(std::shared_ptr<CryptoPP::AutoSeededRandomPool> prng,
          const std::string &filepath);
  bool save(const std::string &filepath) const;
  bool save(Buffer *buffer) const;
  bool load(const Buffer &buffer);
  bool load(const std::string &filepath);
  static constexpr std::size_t sign_length() { return 64; /* TODO  magic */ }
  void sign(const uint8_t *data, const std::size_t size, uint8_t *signature);
  Buffer sign(const uint8_t *data, const std::size_t size);
  Buffer sign(const Buffer &input);

  EccPub make_public_key();
  bool operator==(const EccPriv &key) const;
  friend std::ostream &operator<<(std::ostream &os, const EccPriv &priv);
};

std::ostream &operator<<(std::ostream &os, const EccPriv &priv);

}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_ECCPRIV_HPP */
