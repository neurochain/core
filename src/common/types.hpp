#ifndef NEURO_SRC_COMMON_TYPES_HPP
#define NEURO_SRC_COMMON_TYPES_HPP

#include <boost/asio.hpp>
#include <cassert>
#include <cstdint>
#include <vector>
#include "common/Buffer.hpp"

#include "common.pb.h"

namespace neuro {

namespace networking {
class Peer;
namespace bai = boost::asio::ip;
using IP = bai::address;
using IP4 = bai::address_v4;
using IP6 = bai::address_v6;

enum class ProtocolType { PROTOBUF2, JSON, BSON };

}  // namespace networking

using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::int8_t;

using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

using Port = uint16_t;
using Ports = std::vector<Port>;
const int32_t MessageVersion = 1;

// using Peers = google::protobuf::RepeatedPtrField<neuro::messages::Peer>;

namespace crypto {
namespace keys {
enum class Type { RSA, EC };
}  // namespace keys

namespace hash {
enum class Type { SHA1, SHA_256, SHA3_256 };
}  // namespace hash
}  // namespace crypto

}  // namespace neuro

#endif /* NEURO_SRC_COMMON_TYPES_HPP */
