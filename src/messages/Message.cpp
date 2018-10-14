#include "messages/Message.hpp"
#include "common/logger.hpp"

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
  const auto size = packet.ByteSizeLong();
  buffer->resize(size);
  packet.SerializeToArray(buffer->data(), buffer->size());

  return size;
}

void to_json(const Packet &packet, std::string *output) {
  google::protobuf::util::MessageToJsonString(packet, output);
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
  return json_a == json_b;
}

void hash_transaction(Transaction *transaction) {
  Buffer transaction_serialized;
  messages::to_buffer(*transaction, &transaction_serialized);
  transaction->mutable_id()->CopyFrom(Hasher(transaction_serialized));
}

int32_t fill_header(messages::Header *header) {
  int32_t id = std::rand();
  header->set_version(MessageVersion);
  header->mutable_ts()->set_data(std::time(nullptr));
  header->set_id(id);
  return id;
}

}  // namespace messages
}  // namespace neuro
