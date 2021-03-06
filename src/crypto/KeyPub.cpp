#include "crypto/KeyPub.hpp"
#include <cryptopp/hex.h>
#include <iomanip>
#include <iostream>
#include "common/logger.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace crypto {

KeyPub::KeyPub(const Path &filepath) {
  if (!load(filepath)) {
    throw std::runtime_error("Could not load key from file");
  }
}

KeyPub::KeyPub(const Buffer &key_pub) {
  if (!load(key_pub)) {
    throw std::runtime_error("Could not load key from buffer");
  }
}

KeyPub::KeyPub(const Key &key) : _key(key) {
  // Fill the protobuf
  save(this);
}

KeyPub::Key *KeyPub::key() { return &_key; }

KeyPub::KeyPub(const uint8_t *data, const std::size_t size) {
  if (!load(data, size)) {
    throw std::runtime_error("Could not load key from raw buffer");
  }
}

KeyPub::KeyPub(const messages::_KeyPub &key_pub) {
  if (!load(key_pub)) {
    throw std::runtime_error("Could not load key from proto");
  }
}

bool KeyPub::save(const Path &filepath) const {
  CryptoPP::FileSink fs(filepath.string().c_str(), true);
  _key.Save(fs);

  return true;
}

bool KeyPub::load(const Path &filepath) {
  CryptoPP::FileSource fs(filepath.string().c_str(), true);
  _key.Load(fs);

  // Fill the protobuf
  save(this);

  return true;
}

bool KeyPub::load(const Buffer &buffer) {
  CryptoPP::StringSource array(
      reinterpret_cast<const CryptoPP::byte *>(buffer.data()), buffer.size(),
      true);
  _key.Load(array);

  // Fill the protobuf
  save(this);

  return true;
}

bool KeyPub::load(const uint8_t *data, const std::size_t size) {
  CryptoPP::StringSource array(reinterpret_cast<const CryptoPP::byte *>(data),
                               size, true);
  _key.Load(array);

  // Fill the protobuf
  save(this);

  return true;
}

bool KeyPub::load(const messages::_KeyPub &key_pub) {
  CryptoPP::ECP::Point point;
  _key.AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256k1());

  if (key_pub.has_raw_data()) {
    // Load a compressed public key see
    // https://www.cryptopp.com/wiki/Elliptic_Curve_Digital_Signature_Algorithm
    CryptoPP::StringSource source(key_pub.raw_data(), true);
    _key.GetGroupParameters().GetCurve().DecodePoint(point, source,
                                                     source.MaxRetrievable());

  } else if (key_pub.has_hex_data()) {
    // Compressed public key
    const std::string &hex_data = key_pub.hex_data();

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
  CopyFrom(key_pub);

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
  const auto &x = _key.GetPublicElement().x;
  const auto &y = _key.GetPublicElement().y;

  const auto x_size = 32;
  CryptoPP::byte output[x_size + 1];
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

  return verifier.VerifyMessage((const CryptoPP::byte *)data.data(),
                                data.size(), (const CryptoPP::byte *)signature,
                                size);
}

bool KeyPub::verify(const Buffer &data, const Buffer &signature) const {
  return verify(data, signature.data(), signature.size());
}

bool KeyPub::validate() const {
  CryptoPP::AutoSeededRandomPool prng;
  return _key.Validate(prng, 3);
}

bool KeyPub::operator==(const KeyPub &key) const {
  CryptoPP::ByteQueue queue0, queue1;
  _key.Save(queue0);
  key._key.Save(queue1);
  return queue0 == queue1;
}

bool KeyPub::operator<(const KeyPub &key) const {
  std::string own_raw_data, key_raw_data;
  messages::_KeyPub key_pub;
  if (!has_raw_data()) {
    save(&key_pub);
    own_raw_data = key_pub.raw_data();
  } else {
    own_raw_data = raw_data();
  }
  if (!key.has_raw_data()) {
    save(&key_pub);
    key_raw_data = key_pub.raw_data();
  } else {
    key_raw_data = key.raw_data();
  }
  return own_raw_data < key_raw_data;
}

std::ostream &operator<<(std::ostream &os, const KeyPub &pub) {
  messages::_KeyPub k;
  pub.save(&k);
  os << k;
  return os;
}

}  // namespace crypto
}  // namespace neuro
