#include "api/GRPC.hpp"
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/security/server_credentials.h>

namespace neuro::api {

GRPC::GRPC(Bot *bot) : Api::Api(bot), _monitor(bot) {
  std::string server_address("0.0.0.0:50051");

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(this);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

grpc::Status GRPC::get_status(grpc::ServerContext *context,
                              const google::protobuf::Empty *request,
                              messages::Status *response) {
  auto status = _monitor.fast_status();
  response->CopyFrom(status);
  return grpc::Status::OK;
}

}  // namespace neuro::api
