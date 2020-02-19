#include "api/GRPC.hpp"
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/security/server_credentials.h>
static auto a = logging::core::get()->set_logging_enabled(false);
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

grpc::Status GRPC::balance(grpc::ServerContext *context,
                           const messages::_KeyPub *request,
                           messages::_NCCAmount *response) {
  const auto balance_amount = Api::balance(*request);
  response->CopyFrom(balance_amount);
  return grpc::Status::OK;
}

grpc::Status GRPC::validate_key(grpc::ServerContext *context,
                                const messages::_KeyPub *request,
                                google::protobuf::BoolValue *response) {
  crypto::KeyPub key_pub(*request);
  response->set_value(key_pub.validate());
  return grpc::Status::OK;
}

grpc::Status GRPC::ready(grpc::ServerContext *context,
                         const ::google::protobuf::Empty *request,
                         google::protobuf::BoolValue *response) {
  return grpc::Status::OK;
}
grpc::Status GRPC::list(grpc::ServerContext *context,
                        const ::google::protobuf::Empty *request,
                        messages::_Peers *response) {
  const auto& peers = Api::peers();
  response->CopyFrom(peers);
  return grpc::Status::OK;
}

grpc::Status GRPC::status(grpc::ServerContext *context,
                              const google::protobuf::Empty *request,
                              messages::Status *response) {
  auto status = _monitor.fast_status();
  response->CopyFrom(status);
  return grpc::Status::OK;
}

}  // namespace neuro::api
