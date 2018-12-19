#ifndef NEURO_SRC_NETWORKING_TCP_INBOUNDCONNECTION_HPP
#define NEURO_SRC_NETWORKING_TCP_INBOUNDCONNECTION_HPP

#include "networking/tcp/Connection.hpp"

namespace neuro {
namespace networking {
namespace tcp {
using boost::asio::ip::tcp;

class InboundConnection : public ConnectionBase<InboundConnection> {
 private:
  void read_handshake_message_body(
      PairingCallback pairing_callback, std::size_t body_size);

 public:
  InboundConnection(const std::shared_ptr<crypto::Ecc>& keys,
                    const std::shared_ptr<messages::Queue>& queue,
                    const std::shared_ptr<tcp::socket>& socket,
                    UnpairingCallback unpairing_callback);
  void read_hello(PairingCallback pairing_callback);
};
}  // namespace tcp
}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_TCP_INBOUNDCONNECTION_HPP */
