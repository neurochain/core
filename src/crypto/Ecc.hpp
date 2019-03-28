#ifndef NEURO_SRC_CRYPTO_ECC_HPP
#define NEURO_SRC_CRYPTO_ECC_HPP

#include <memory>

#include "common/types.hpp"
#include "crypto/KeyPriv.hpp"
#include "crypto/KeyPub.hpp"
#include "messages/config/Config.hpp"

namespace neuro {
namespace crypto {

namespace test {
class Ecc;
}  // namespace test

class Ecc {
 public:
 private:
  std::shared_ptr<CryptoPP::AutoSeededRandomPool> _prng;
  std::unique_ptr<KeyPriv> _key_private;
  std::unique_ptr<KeyPub> _key_public;

  bool load_keys(const std::string &keypath_priv,
                 const std::string &keypath_pub);

 public:
  Ecc();
  Ecc(Ecc &&ecc) {}
  Ecc(const Ecc &) = delete;
  Ecc(const std::string &filepath_private, const std::string &filepath_public);
  Ecc(const messages::_KeyPriv &key_priv, const messages::_KeyPub &key_pub);

  const KeyPriv &key_priv() const;
  const KeyPub &key_pub() const;

  KeyPriv *mutable_key_priv();
  KeyPub *mutable_key_pub();

  bool operator==(const Ecc &ecc) const;

  bool save(const std::string &filepath_private,
            const std::string &filepath_public) const;
  Buffer sign(const Buffer &input);
  Buffer sign(const uint8_t *data, const std::size_t size);
  void sign(const uint8_t *data, const std::size_t size, uint8_t *dest);
  static constexpr std::size_t sign_length() { return KeyPriv::sign_length(); }
  friend class test::Ecc;
};

class Eccs : public std::vector<Ecc> {
 public:
  Eccs(const messages::config::Networking &networking) {
    for (const auto &keys_paths : networking.keys_paths()) {
      emplace_back(keys_paths.key_priv_path(), keys_paths.key_pub_path());
    }
    for (const auto &keys : networking.keys()) {
      emplace_back(keys.key_priv(), keys.key_pub());
    }
  }
};

}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_ECC_HPP */
