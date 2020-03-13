#include "GRPC.hpp"
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

namespace neuro::api {

GRPC::GRPC(const messages::config::GRPC &config, Bot *bot)
    : Api::Api(bot),
      _block_service(bot),
      _transaction_service(bot),
      _status_service(bot) {
  _server_thread = std::thread([this, &config](){
    std::string server_address("0.0.0.0:" + std::to_string(config.port()));
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, ::grpc::InsecureServerCredentials());
    builder.RegisterService(&_block_service);
    builder.RegisterService(&_transaction_service);
    builder.RegisterService(&_status_service);
    _server = builder.BuildAndStart();
    _server->Wait();
  });
}

GRPC::~GRPC() {
  _server->Shutdown();
  _server_thread.join();
}

}  // namespace neuro::api
