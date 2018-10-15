#ifndef NEURO_SRC_MESSAGES_MESSAGE_HPP
#define NEURO_SRC_MESSAGES_MESSAGE_HPP

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <fstream>
#include <string>
#include "common/types.hpp"
#include "crypto/EccPriv.hpp"
#include "messages.pb.h"
#include "mongo/mongo.hpp"

namespace neuro {
namespace messages {

using BlockHeight = decltype(((BlockHeader *)nullptr)->height());
using BlockID = decltype(((BlockHeader *)nullptr)->id());
using TransactionID = decltype(((Transaction *)nullptr)->id());
using Packet = google::protobuf::Message;
using Type = Body::BodyCase;

Type get_type(const Body &body);

bool from_buffer(const Buffer &buffer, Packet *packet);
bool from_json(const std::string &json, Packet *packet);
bool from_json_file(const std::string &path, Packet *packet);
bool from_bson(const bsoncxx::document::value &doc, Packet *packet);
bool from_bson(const bsoncxx::document::view &doc, Packet *packet);

std::size_t to_buffer(const Packet &packet, Buffer *buffer);
void to_json(const Packet &packet, std::string *output);
bsoncxx::document::value to_bson(const Packet &packet);
std::ostream &operator<<(std::ostream &os, const Packet &packet);

bool operator==(const Packet &a, const Packet &b);

void hash_transaction(Transaction *transaction);
int32_t fill_header(messages::Header *header);
int32_t fill_header_reply(const messages::Header &header_request,
                          messages::Header *header_reply);

}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_MESSAGE_HPP */
