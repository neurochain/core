#ifndef NEURO_SRC_NETWORKING_TCP_OUTBOUNDCONNECTION_HPP
#define NEURO_SRC_NETWORKING_TCP_OUTBOUNDCONNECTION_HPP

#include <functional>

#include "networking/PeerPool.hpp"
#include "networking/tcp/Connection.hpp"

namespace neuro {
namespace networking {
namespace tcp {

using boost::asio::ip::tcp;

class OutboundConnection : public Connection {
  private:
   void handshake_init(PairingCallback pairing_callback);
   void handshake_ack(PairingCallback pairing_callback);
   messages::Message build_hello_message() const;
   void read_ack(PairingCallback pairing_callback);
   void read_handshake_message_body(PairingCallback pairing_callback,
                                    std::size_t body_size);

  public:
   OutboundConnection(const std::shared_ptr<crypto::Ecc>& keys,
                      const std::shared_ptr<messages::Queue>& queue,
                      const std::shared_ptr<tcp::socket>& socket,
                      const std::shared_ptr<messages::Peer>& peer,
                      PairingCallback pairing_callback,
                      UnpairingCallback unpairing_callback);
};

}  // namespace tcp
}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_TCP_OUTBOUNDCONNECTION_HPP */
