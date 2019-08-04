#include "messages/Message.hpp"
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
    std::stringstream error;
    error << "Could not parse json " << r;
    LOG_ERROR << error.str() << std::endl
              << boost::stacktrace::stacktrace() << std::endl;
    throw std::runtime_error(error.str());
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

bool to_buffer(const Packet &packet, Buffer *buffer) {
  try {
    const auto size = packet.ByteSizeLong();
    buffer->resize(size);
    packet.SerializeToArray(buffer->data(), buffer->size());
    return true;
  } catch (...) {
    LOG_WARNING << boost::stacktrace::stacktrace() << std::endl;
    return false;
  }
}

std::optional<Buffer> to_buffer(const Packet &packet) {
  Buffer buffer;
  if(!to_buffer(packet, &buffer)) {
    return std::nullopt;
  }
  return std::make_optional(buffer);
}

void to_json(const Packet &packet, std::string *output) {
  try {
    google::protobuf::util::MessageToJsonString(packet, output);
  } catch (...) {
    auto buff = to_buffer(packet);
    if(!buff) {
      throw std::runtime_error("Could not parse packet");
    }
    buff->save("crashing.proto");
    LOG_ERROR << "Could not to_json packet " << buff->size() << boost::stacktrace::stacktrace()
              << std::endl;
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

void sort_transactions(Block *block) {
  // We use to_json to sort because the order need to be the same as in mongodb.
  // If we sorted by the raw content of the data field it would give a different
  // order.
  std::sort(
      block->mutable_transactions()->begin(),
      block->mutable_transactions()->end(),
      [](const Transaction &transaction0, const Transaction &transaction1) {
        return to_json(transaction0.id()) < to_json(transaction1.id());
      });
}

void set_transaction_hash(Transaction *transaction) {
  // Fill the id which is a required field. This makes the transaction
  // serializable.
  transaction->mutable_id()->set_type(messages::Hash::SHA256);
  transaction->mutable_id()->set_data("");

  const auto id = messages::Hasher(*transaction);
  transaction->mutable_id()->CopyFrom(id);
}

void set_default(messages::Signature *author) {
  author->Clear();
  author->mutable_key_pub()->set_raw_data("");
  author->mutable_signature()->set_type(messages::Hash::SHA256);
  author->mutable_signature()->set_data("");
}

void set_block_hash(Block *block) {
  auto header = block->mutable_header();

  // Fill the id which is a required field. This makes the block
  // serializable.
  header->mutable_id()->set_type(messages::Hash::SHA256);
  header->mutable_id()->set_data("");
  
  // The author should always be filled after the hash is set because the author
  // field contains the signature of a denunciation that contains the hash of
  // the block
  set_default(header->mutable_author());

  const auto id = messages::Hasher(*block);
  header->mutable_id()->CopyFrom(id);
}

int32_t fill_header(messages::Header *header) {
  const auto id = _dist(_rd);
  header->set_version(MessageVersion);
  header->mutable_ts()->set_data(std::time(nullptr));
  header->set_id(id);
  return id;
}

int32_t fill_header_reply(const messages::Header &header_request,
                          messages::Header *header_reply) {
  const auto id = fill_header(header_reply);
  header_reply->set_connection_id(header_request.connection_id());
  header_reply->set_request_id(header_request.id());
  return id;
}

}  // namespace messages
}  // namespace neuro
