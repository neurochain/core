#include "messages/Message.hpp"
#include <boost/stacktrace.hpp>
#include "common/logger.hpp"
#include "messages/Hasher.hpp"

namespace neuro {
namespace messages {

Type get_type(const Body &body) { return body.body_case(); }

bool from_buffer(const Buffer &buffer, Packet *packet) {
  return packet->ParseFromArray(buffer.data(), buffer.size());
}

bool from_json(const std::string &json, Packet *packet) {
  google::protobuf::util::JsonParseOptions options;
  auto r = google::protobuf::util::JsonStringToMessage(json, packet, options);
  if (!r.ok()) {
    LOG_ERROR << "Could not parse json " << r;
  }
  return r.ok();
}

bool from_json_file(const std::string &path, Packet *packet) {
  std::ifstream t(path);
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  return from_json(str, packet);
}

bool from_bson(const bsoncxx::document::value &doc, Packet *packet) {
  const auto mongo_json = bsoncxx::to_json(doc);
  return from_json(mongo_json, packet);
}

bool from_bson(const bsoncxx::document::view &doc, Packet *packet) {
  const auto mongo_json = bsoncxx::to_json(doc);
  return from_json(mongo_json, packet);
}

std::size_t to_buffer(const Packet &packet, Buffer *buffer) {
  try {
    const auto size = packet.ByteSizeLong();
    buffer->resize(size);
    packet.SerializeToArray(buffer->data(), buffer->size());
    return size;
  } catch (...) {
    std::cout << boost::stacktrace::stacktrace() << std::endl;
    throw;
  }
}

void to_json(const Packet &packet, std::string *output) {
  try {
    google::protobuf::util::MessageToJsonString(packet, output);
  } catch (...) {
    std::cout << boost::stacktrace::stacktrace() << std::endl;
    throw;
  }
}

std::string to_json(const Packet &packet) {
  std::string output;
  to_json(packet, &output);
  return output;
}

bsoncxx::document::value to_bson(const Packet &packet) {
  std::string json;
  to_json(packet, &json);
  return bsoncxx::from_json(json);
}

std::ostream &operator<<(std::ostream &os, const Packet &packet) {
  std::string buff;
  to_json(packet, &buff);
  os << buff;
  return os;
}

bool operator==(const Packet &a, const Packet &b) {
  std::string json_a, json_b;
  to_json(a, &json_a);
  to_json(b, &json_b);
  bool res = json_a == json_b;

  return res;
}

bool operator!=(const Packet &a, const Packet &b) { return !(a == b); }

bool operator==(const messages::Peer &a, const messages::Peer &b) {
  LOG_TRACE << a.endpoint() << " " << b.endpoint();
  return a.endpoint() == b.endpoint() && a.port() == b.port();
}

bool operator!=(const messages::Peer &a, const messages::Peer &b) {
  return !(a == b);
}

void set_transaction_hash(Transaction *transaction) {
  // Fill the id which is a required field. This makes the transaction
  // serializable.
  transaction->mutable_id()->set_type(messages::Hash::SHA256);
  transaction->mutable_id()->set_data("");

  const auto id = messages::Hasher(*transaction);
  transaction->mutable_id()->CopyFrom(id);
}

void set_block_hash(Block *block) {
  auto header = block->mutable_header();

  // Fill the id which is a required field. This makes the block
  // serializable.
  header->mutable_id()->set_type(messages::Hash::SHA256);
  header->mutable_id()->set_data("");

  const auto id = messages::Hasher(*block);
  header->mutable_id()->CopyFrom(id);
}

int32_t fill_header(messages::Header *header) {
  int32_t id = std::rand();
  header->set_version(MessageVersion);
  header->mutable_ts()->set_data(std::time(nullptr));
  header->set_id(id);
  return id;
}

int32_t fill_header_reply(const messages::Header &header_request,
                          messages::Header *header_reply) {
  const auto id = fill_header(header_reply);
  header_reply->set_request_id(header_request.id());
  header_reply->mutable_peer()->CopyFrom(header_request.peer());
  return id;
}

}  // namespace messages
}  // namespace neuro
