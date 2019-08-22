#ifndef NEURO_SRC_COMMON_TYPES_HPP
#define NEURO_SRC_COMMON_TYPES_HPP

#include <mpreal.h>
#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/filesystem/path.hpp>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <random>
#include <vector>

#include "common.pb.h"
#include "common/Buffer.hpp"

namespace neuro {
extern bool _fake_time;
std::time_t time();
std::time_t time(std::time_t increment);

static std::random_device _rd;  // TODO make global
static std::uniform_int_distribution<uint32_t> _dist{
    0, std::numeric_limits<uint32_t>::max()};

using namespace std::chrono_literals;

using namespace std::chrono_literals;

namespace networking {
class Peer;

namespace bai = boost::asio::ip;
using IP = bai::address;
using IP4 = bai::address_v4;
using IP6 = bai::address_v6;

enum class ProtocolType { PROTOBUF2, JSON, BSON };

}  // namespace networking

namespace consensus {
using Address = std::string;
}  // namespace consensus

using Path = boost::filesystem::path;

using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::int8_t;

using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;

using Duration = std::chrono::seconds;

using Endpoint = std::string;
using Port = uint16_t;
using Ports = std::vector<Port>;
const int32_t MessageVersion = 4;
const int32_t MESSAGE_TTL = 60;
const int32_t MAX_MESSAGE_SIZE = 256 * 1024;

// using Peers = google::protobuf::RepeatedPtrField<neuro::messages::Peer>;

// This is used both in the ledger and the consensus
using Double = mpfr::mpreal;

static std::recursive_mutex mpfr_mutex;

// Useful debug utilities
using Timer = std::chrono::high_resolution_clock;

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
