
#include "Ecc.hpp"
#include <cryptopp/asn.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/files.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha3.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "common/logger.hpp"
#include "crypto/KeyPriv.hpp"
namespace neuro {
namespace crypto {
static constexpr std::size_t sign_length() { return KeyPriv::sign_length(); }

Ecc::Ecc()
    : _prng(std::make_shared<CryptoPP::AutoSeededRandomPool>()),
      _key_private(std::make_unique<KeyPriv>(_prng)),
      _key_public(std::make_unique<KeyPub>(_key_private->make_key_pub())) {}

Ecc::Ecc(const std::string &filepath_private,
         const std::string &filepath_public)
    : _prng(std::make_shared<CryptoPP::AutoSeededRandomPool>()) {
  if (!load_keys(filepath_private, filepath_public)) {
    throw std::runtime_error("Could not create or load keys");
  }
}

Ecc::Ecc(const messages::_KeyPriv &key_priv, const messages::_KeyPub &key_pub)
    : _prng(std::make_shared<CryptoPP::AutoSeededRandomPool>()),
      _key_private(std::make_unique<KeyPriv>(_prng, key_priv)),
      _key_public(std::make_unique<KeyPub>(key_pub)) {}

bool Ecc::load_keys(const std::string &keypath_priv,
                    const std::string &keypath_pub) {
  bool keys_save{false};
  bool keys_create{false};

  if (!boost::filesystem::exists(keypath_pub) ||
      !boost::filesystem::exists(keypath_priv)) {
    keys_create = true;
    keys_save = true;
  }

  if (!keys_create) {
    LOG_INFO << this << " Loading keys from " << keypath_priv << " and "
             << keypath_pub;
    _key_private = std::make_unique<KeyPriv>(_prng, keypath_priv);
    _key_public = std::make_unique<KeyPub>(keypath_pub);
    return true;
  }

  LOG_INFO << this << " Generating new keys";
  _key_private = std::make_unique<KeyPriv>(_prng);
  _key_public = std::make_unique<KeyPub>(_key_private->make_key_pub());

  if (keys_save) {
    LOG_INFO << this << " Saving keys to " << keypath_priv << " and "
             << keypath_pub;
    save(keypath_priv, keypath_pub);
  }

  return true;
}

const KeyPriv &Ecc::key_priv() const { return *_key_private.get(); }
const KeyPub &Ecc::key_pub() const { return *_key_public.get(); }
KeyPriv *Ecc::mutable_key_priv() { return _key_private.get(); }
KeyPub *Ecc::mutable_key_pub() { return _key_public.get(); }

bool Ecc::save(const std::string &filepath_private,
               const std::string &filepath_public) const {
  if (!_key_private->save(filepath_private)) {
    LOG_ERROR << "Could not save private key file";
    return false;
  }

  if (!_key_public->save(filepath_public)) {
    LOG_ERROR << "Could not save public key file";
    return false;
  }

  return true;
}

Buffer Ecc::sign(const Buffer &input) { return _key_private->sign(input); }

Buffer Ecc::sign(const uint8_t *data, const std::size_t size) {
  return _key_private->sign(data, size);
}

void Ecc::sign(const uint8_t *data, const std::size_t size, uint8_t *dest) {
  return _key_private->sign(data, size, dest);
}

bool Ecc::operator==(const Ecc &ecc) const {
  return (ecc.key_priv() == *_key_private.get() &&
          ecc.key_pub() == *_key_public.get());
}

}  // namespace crypto
}  // namespace neuro
