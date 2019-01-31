#ifndef NEURO_SRC_MESSAGES_MESSAGE_HPP
#define NEURO_SRC_MESSAGES_MESSAGE_HPP

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <fstream>
#include <functional>
#include <string>

#include "common/Buffer.hpp"
#include "common/types.hpp"
#include "crypto/EccPriv.hpp"
#include "ledger/mongo.hpp"
#include "messages.pb.h"
#include "messages/Peer.hpp"

namespace neuro {
namespace messages {

using BlockHeight = decltype(((BlockHeader *)nullptr)->height());
using BlockID = decltype(((BlockHeader *)nullptr)->id());
using TransactionID = decltype(((Transaction *)nullptr)->id());
using Packet = google::protobuf::Message;

using Type = Body::BodyCase;
using BranchID = uint32_t;
using BlockScore = double;

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

bool operator==(const Packet &a, const Packet &b);
bool operator==(const messages::Peer &a, const messages::Peer &b);

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
  NCCAmount(uint64_t amount) { set_value(amount); }
};

}  // namespace messages
}  // namespace neuro

namespace std {
template <>
struct hash<neuro::messages::Packet> {
  using argument_type = neuro::messages::Packet;
  using result_type = size_t;

  result_type operator()(argument_type const &packet) const noexcept {
    neuro::Buffer buffer;
    neuro::messages::to_buffer(packet, &buffer);
    const auto hash = std::hash<neuro::Buffer>{}(buffer);
    return hash;
  }
};

template <>
struct hash<neuro::messages::KeyPub> {
  using argument_type = neuro::messages::KeyPub;
  using result_type = size_t;

  result_type operator()(argument_type const &packet) const noexcept {
    neuro::Buffer buffer;
    neuro::messages::to_buffer(packet, &buffer);
    const auto hash = std::hash<neuro::Buffer>{}(buffer);
    return hash;
  }
};

}  // namespace std

#endif /* NEURO_SRC_MESSAGES_MESSAGE_HPP */
