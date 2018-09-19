#ifndef NEURO_SRC_COMMON_TYPES_HPP
#define NEURO_SRC_COMMON_TYPES_HPP

#include <cstdint>
#include <vector>
#include <boost/asio.hpp>
#include "common/Buffer.hpp"

namespace neuro {

namespace networking {
class Peer;
namespace bai = boost::asio::ip;
using IP  = bai::address;
using IP4 = bai::address_v4;
using IP6 = bai::address_v6;

}

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
