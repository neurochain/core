#include "networking/Peer.hpp"

namespace neuro {
namespace networking {

bool operator==(const Peer &a, const Peer &b) {
  // bool same = a.port() == b.port() && a.ip() == b.ip();
  // return a.key_pub() && b.key_pub() ? same && a.key_pub() == b.key_pub() :
  // same;
  return a.port() == b.port() && a.ip() == b.ip();
}

std::ostream &operator<<(std::ostream &os, const Peer &peer) {
  std::string state;
  switch (peer.status()) {
  case Peer::Status::CONNECTED:
    state = "Connected";
    break;
  case Peer::Status::CONNECTING:
    state = "Connecting";
    break;
  case Peer::Status::REACHABLE:
    state = "Reachable";
    break;
  case Peer::Status::UNREACHABLE:
    state = "Unreachable";
    break;
  case Peer::Status::FULL:
    state = "Full";
    break;
  }
  os << peer.ip() << ":" << peer.port() << " (" << state << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const Peers &peers) {
  os << "{";
  size_t __peers_size = peers.size();
  for (size_t i = 0; i < __peers_size; i += 1) {
    const auto &peer = peers[i];
    os << "[" << *peer << "]";
    if (i != __peers_size - 1)
      os << ",";
  }
  os << "}";
  return os;
}

bool operator==(const Peers &a, const Peers &b) {
  if (a.size() != b.size()) {
    return false;
  }

  auto ita = a.begin();
  auto itb = b.begin();
  auto end = a.end();

  while (ita != end) {

    if (!(*(*ita) == *(*itb))) {
      return false;
    }

    ++ita;
    ++itb;
  }

  return true;
}

} // namespace networking
} // namespace neuro
