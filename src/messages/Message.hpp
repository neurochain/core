#ifndef NEURO_SRC_MESSAGES_MESSAGE_HPP
#define NEURO_SRC_MESSAGES_MESSAGE_HPP

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <fstream>
#include <functional>
#include <string>
#include <type_traits>

#include "common/Buffer.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "consensus.pb.h"
#include "crypto/KeyPriv.hpp"
#include "ledger/mongo.hpp"
#include "messages.pb.h"
#include "messages/Peer.hpp"

namespace neuro {
namespace messages {

using NCCValue = decltype(((_NCCAmount *)nullptr)->value());
using BlockHeight = decltype(((BlockHeader *)nullptr)->height());
using BlockScore = decltype(((TaggedBlock *)nullptr)->score());
using AssemblyHeight = decltype(((Assembly *)nullptr)->height());
using BlockID = std::remove_reference<decltype(
    *(((BlockHeader *)nullptr)->mutable_id()))>::type;
using AssemblyID = std::remove_reference<decltype(
    *(((Assembly *)nullptr)->mutable_id()))>::type;
using TransactionID = std::remove_reference<decltype(
    *(((Transaction *)nullptr)->mutable_id()))>::type;
using Packet = google::protobuf::Message;

using Type = Body::BodyCase;
using BranchID = int32_t;
using IntegrityScore = Double;

Type get_type(const Body &body);

bool from_buffer(const Buffer &buffer, Packet *packet);
bool from_json(const std::string &json, Packet *packet);
bool from_json_file(const std::string &path, Packet *packet);
bool from_bson(const bsoncxx::document::value &doc, Packet *packet);
bool from_bson(const bsoncxx::document::view &doc, Packet *packet);

std::size_t to_buffer(const Packet &packet, Buffer *buffer);
void to_json(const Packet &packet, std::string *output);
std::string to_json(const Packet &packet);
bsoncxx::document::value to_bson(const Packet &packet);
std::ostream &operator<<(std::ostream &os, const Packet &packet);

template <typename T>
std::ostream &operator<<(
    std::ostream &os, const ::google::protobuf::RepeatedPtrField<T> &packets) {
  for (const auto &packet : packets) {
    os << packet << std::endl;
  }
  return os;
}

void sort_transactions(Block *block);
void set_default(Signature *author);
void set_transaction_hash(Transaction *transaction);
void set_block_hash(Block *block);
int32_t fill_header(messages::Header *header);
int32_t fill_header_reply(const messages::Header &header_request,
                          messages::Header *header_reply);

class Message : public _Message {
 public:
  Message() { fill_header(mutable_header()); }
  Message(const std::string &json) { from_json(json, this); }

  Message(const Path &path) { from_json_file(path.string(), this); }
  virtual ~Message() {}
};

class NCCAmount : public _NCCAmount {
 public:
  NCCAmount() {}
  NCCAmount(const _NCCAmount &nccsdf) : _NCCAmount(nccsdf) {}
  NCCAmount(int64_t amount) { set_value(amount); }
};

class Denunciation : public _Denunciation {
 public:
  Denunciation() {}
  Denunciation(const _Denunciation &denunciation)
      : _Denunciation(denunciation) {}
  Denunciation(const Block &block) {
    mutable_block_id()->CopyFrom(block.header().id());
    set_block_height(block.header().height());
    mutable_block_author()->CopyFrom(block.header().author());
  }
};

template <typename TA, typename TB>
typename std::enable_if<
    std::is_base_of<::neuro::messages::Packet, TA>::value &&
        std::is_base_of<::neuro::messages::Packet, TB>::value,
    bool>::type
operator==(const TA &a, const TB &b) {
  std::string json_a, json_b;
  to_json(a, &json_a);
  to_json(b, &json_b);
  bool res = json_a == json_b;
  return res;
}

template <typename TA, typename TB>
typename std::enable_if<
    std::is_base_of<::neuro::messages::Packet, TA>::value &&
        std::is_base_of<::neuro::messages::Packet, TB>::value,
    bool>::type
operator!=(const TA &a, const TB &b) {
  return !(a == b);
}

}  // namespace messages
}  // namespace neuro

template <typename T>
struct PacketHash {
  std::size_t operator()(typename std::enable_if<
                         std::is_base_of<::neuro::messages::Packet, T>::value,
                         T>::type const &s) const noexcept {
    return std::hash<std::string>()(neuro::messages::to_json(s));
  }
};

template <>
struct PacketHash<neuro::messages::_KeyPub> {
  std::size_t operator()(neuro::messages::_KeyPub const &s) const noexcept {
    size_t key_as_bytes = *s.raw_data().data();
    return key_as_bytes;
  }
};

namespace std {
template <>
struct hash<neuro::messages::Input> {
  size_t operator()(const neuro::messages::Input &input) const {
    return hash<string>()(neuro::messages::to_json(input));
  }
};

template <>
struct hash<neuro::messages::Hash> {
  size_t operator()(const neuro::messages::Hash &hash_) const {
    return hash<string>()(neuro::messages::to_json(hash_));
  }
};

template <>
struct hash<neuro::messages::TaggedTransaction> {
  size_t operator()(
      const neuro::messages::TaggedTransaction &tagged_transaction) const {
    return hash<string>()(neuro::messages::to_json(tagged_transaction));
  }
};

}  // namespace std

#endif /* NEURO_SRC_MESSAGES_MESSAGE_HPP */
