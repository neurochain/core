
#include "Ecc.hpp"
#include <asn.h>
#include <eccrypto.h>
#include <files.h>
#include <oids.h>
#include <osrng.h>
#include <sha3.h>

#include "common/logger.hpp"

namespace neuro {
namespace crypto {

Ecc::Ecc()
    : _prng(std::make_shared<CryptoPP::AutoSeededRandomPool>()),
      _key_private(_prng),
      _key_public(_key_private.make_key_pub()) {}

Ecc::Ecc(const std::string &filepath_private,
         const std::string &filepath_public)
    : _prng(std::make_shared<CryptoPP::AutoSeededRandomPool>()),
      _key_private(_prng, filepath_private),
      _key_public(filepath_public) {}

Ecc::Ecc(const KeyPriv &ecc_priv, const KeyPub &ecc_pub)
    : _prng(std::make_shared<CryptoPP::AutoSeededRandomPool>()),
      _key_private(ecc_priv),
      _key_public(ecc_pub) {}

const KeyPriv &Ecc::key_priv() const { return _key_private; }
const KeyPub &Ecc::key_pub() const { return _key_public; }
KeyPriv *Ecc::mutable_key_priv() { return &_key_private; }
KeyPub *Ecc::mutable_key_pub() { return &_key_public; }

bool Ecc::save(const std::string &filepath_private,
               const std::string &filepath_public) const {
  if (!_key_private.save(filepath_private)) {
    LOG_ERROR << "Could not save private key file";
    return false;
  }

  if (!_key_public.save(filepath_public)) {
    LOG_ERROR << "Could not save public key file";
    return false;
  }

  return true;
}

Buffer Ecc::sign(const Buffer &input) { return _key_private.sign(input); }

Buffer Ecc::sign(const uint8_t *data, const std::size_t size) {
  return _key_private.sign(data, size);
}

void Ecc::sign(const uint8_t *data, const std::size_t size, uint8_t *dest) {
  return _key_private.sign(data, size, dest);
}

bool Ecc::operator==(const Ecc &ecc) const {
  return (ecc.key_priv() == _key_private && ecc.key_pub() == _key_public);
}

}  // namespace crypto
}  // namespace neuro
