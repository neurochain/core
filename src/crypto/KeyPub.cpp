#include "crypto/KeyPub.hpp"
#include <cryptopp/hex.h>
#include <iomanip>
#include <iostream>
#include "common/logger.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace crypto {

KeyPub::KeyPub(const std::string &filepath) { load(filepath); }

KeyPub::KeyPub(const Buffer &key_pub) {
  if (!load(key_pub)) {
    throw std::runtime_error("Could not load key from buffer");
  }
}

KeyPub::KeyPub(const Key &key) : _key(key) {}

KeyPub::KeyPub(const messages::_KeyPub &key_pub) { load(key_pub); }

KeyPub::Key *KeyPub::key() { return &_key; }

bool KeyPub::save(const std::string &filepath) const {
  CryptoPP::FileSink fs(filepath.c_str(), true);
  _key.Save(fs);

  return true;
}

bool KeyPub::load(const std::string &filepath) {
  CryptoPP::FileSource fs(filepath.c_str(), true);
  _key.Load(fs);

  return true;
}

bool KeyPub::load(const Buffer &buffer) {
  CryptoPP::StringSource array(reinterpret_cast<const byte *>(buffer.data()),
                               buffer.size(), true);
  _key.Load(array);
  return true;
}

bool KeyPub::load(const uint8_t *data, const std::size_t size) {
  CryptoPP::StringSource array(reinterpret_cast<const byte *>(data), size,
                               true);
  _key.Load(array);
  return true;
}

bool KeyPub::load(const messages::_KeyPub &keypub) {
  CryptoPP::ECP::Point point;
  _key.AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256k1());

  if (keypub.has_raw_data()) {
    // Load a compressed public key see
    // https://www.cryptopp.com/wiki/Elliptic_Curve_Digital_Signature_Algorithm
    CryptoPP::StringSource source(keypub.raw_data(), true);
    _key.GetGroupParameters().GetCurve().DecodePoint(point, source,
                                                     source.MaxRetrievable());

  } else if (keypub.has_hex_data()) {
    // Compressed public key
    const std::string &hex_data = keypub.hex_data();

    // We need to have a new here because cryptoPP will try to delete the
    // decoder
    CryptoPP::StringSource ss(hex_data, true, new CryptoPP::HexDecoder);
    _key.GetGroupParameters().GetCurve().DecodePoint(point, ss,
                                                     ss.MaxRetrievable());
  } else {
    // not possible since we have a oneof
    return false;
  }
  _key.Initialize(CryptoPP::ASN1::secp256k1(), point);

  return true;
}

Buffer KeyPub::save() const {
  Buffer tmp;
  std::string s;
  _key.Save(CryptoPP::StringSink(s).Ref());
  tmp.copy(s);
  return tmp;
}

bool KeyPub::save(Buffer *buffer) const {
  std::string s;
  _key.Save(CryptoPP::StringSink(s).Ref());
  buffer->copy(s);
  return true;
}

bool KeyPub::save(messages::_KeyPub *key_pub) const {
  key_pub->set_type(messages::KeyType::ECP256K1);
  const auto x = _key.GetPublicElement().x;
  const auto y = _key.GetPublicElement().y;

  const auto x_size = 32;
  byte output[x_size + 1];
  x.Encode(&output[1], x_size);

  // Set compressed flag see
  // https://www.cryptopp.com/wiki/Elliptic_Curve_Digital_Signature_Algorithm
  // for more details
  if (y.IsEven()) {
    output[0] = 0x02;
  } else {
    output[0] = 0x03;
  }

  key_pub->set_raw_data(
      std::string(reinterpret_cast<char *>(output), x_size + 1));
  return true;
}

bool KeyPub::save_as_hex(messages::_KeyPub *key_pub) const {
  key_pub->set_type(messages::KeyType::ECP256K1);
  const auto x = _key.GetPublicElement().x;
  const auto y = _key.GetPublicElement().y;

  // Sadly I do need to use two stream variables...
  std::stringstream hex_stream, padded_stream;
  hex_stream << std::hex << x;
  padded_stream << std::setfill('0') << std::setw(65) << hex_stream.str();

  std::string compressed_flag = y.IsEven() ? "02" : "03";
  key_pub->set_hex_data(compressed_flag + padded_stream.str());
  return true;
}

bool KeyPub::verify(const Buffer &data, const uint8_t *signature,
                    const std::size_t size) const {
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Verifier verifier(_key);

  return verifier.VerifyMessage((const byte *)data.data(), data.size(),
                                (const byte *)signature, size);
}

bool KeyPub::verify(const Buffer &data, const Buffer &signature) const {
  return verify(data, signature.data(), signature.size());
}

bool KeyPub::operator==(const KeyPub &key) const {
  CryptoPP::ByteQueue queue0;
  CryptoPP::ByteQueue queue1;
  _key.Save(queue0);
  key._key.Save(queue1);
  return (queue0 == queue1);
}

std::ostream &operator<<(std::ostream &os, const KeyPub &pub) {
  messages::_KeyPub k;
  pub.save(&k);
  os << k;
  return os;
}
}  // namespace crypto
}  // namespace neuro
