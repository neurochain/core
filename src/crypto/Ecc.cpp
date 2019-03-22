
#include "Ecc.hpp"
#include <asn.h>
#include <eccrypto.h>
#include <files.h>
#include <oids.h>
#include <osrng.h>
#include <sha3.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "common/logger.hpp"

namespace neuro {
namespace crypto {

Ecc::Ecc()
    : _prng(std::make_shared<CryptoPP::AutoSeededRandomPool>()),
      _key_private(std::make_unique<EccPriv>(_prng)),
      _key_public(std::make_unique<EccPub>(_key_private->make_public_key())) {}

Ecc::Ecc(const std::string &filepath_private,
         const std::string &filepath_public)
    : _prng(std::make_shared<CryptoPP::AutoSeededRandomPool>()) {
  if (!load_keys(filepath_private, filepath_public)) {
    throw std::runtime_error("Could not create or load keys");
  }
}

Ecc::Ecc(const EccPriv &ecc_priv, const EccPub &ecc_pub)
    : _prng(std::make_shared<CryptoPP::AutoSeededRandomPool>()),
      _key_private(std::make_unique<EccPriv>(_prng)),
      _key_public(std::make_unique<EccPub>(_key_private->make_public_key())) {}

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
    _key_private = std::make_unique<EccPriv>(_prng, keypath_priv);
    _key_public = std::make_unique<EccPub>(keypath_pub);
    return true;
  }

  LOG_INFO << this << " Generating new keys";
  _key_private = std::make_unique<EccPriv>(_prng);
  _key_public = std::make_unique<EccPub>(_key_private->make_public_key());

  if (keys_save) {
    LOG_INFO << this << " Saving keys to " << keypath_priv << " and "
             << keypath_pub;
    save(keypath_priv, keypath_pub);
  }

  return true;
}

// Ecc::Ecc(const EccPriv &ecc_priv, const EccPub &ecc_pub)
//     : _prng(std::make_shared<CryptoPP::AutoSeededRandomPool>()),
//       _key_private(ecc_priv),
//       _key_public(ecc_pub) {}

const EccPriv &Ecc::private_key() const { return *_key_private.get(); }
const EccPub &Ecc::public_key() const { return *_key_public.get(); }
EccPriv *Ecc::mutable_private_key() { return _key_private.get(); }
EccPub *Ecc::mutable_public_key() { return _key_public.get(); }

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
  return (ecc.private_key() == *_key_private.get() &&
          ecc.public_key() == *_key_public.get());
}

}  // namespace crypto
}  // namespace neuro
