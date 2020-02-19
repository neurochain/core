#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "messages/service.grpc.pb.h"
#include "messages/Message.hpp"

namespace neuro::api::test {
}  // namespace neuro::api::test

using namespace neuro;

int main() {
  auto channel = CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
  auto stub = grpcservice::Status::NewStub(channel);
  grpc::ClientContext ctx;
  messages::Status response;
  stub->status(&ctx, google::protobuf::Empty(), &response);
  std::cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ": "
            << to_json(response, true)
            << std::endl;
  return 0;
}
