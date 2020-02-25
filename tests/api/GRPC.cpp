#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "messages/service.grpc.pb.h"
#include "messages/Message.hpp"

namespace neuro::api::test {
}  // namespace neuro::api::test

using namespace neuro;

int main() {
  auto channel = CreateChannel("localhost:8081", grpc::InsecureChannelCredentials());
  auto stub = grpcservice::Block::NewStub(channel);
  grpc::ClientContext ctx;
  messages::Block response;
  auto reader = stub->watch(&ctx, google::protobuf::Empty());
  int new_block_handled = 0;
  while(reader->Read(&response)) {
    new_block_handled++;
    std::cout << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ": "
              << new_block_handled
              << std::endl;
  }
  auto status = reader->Finish();
  std::cout << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ": "
            << status.ok() << " : " << status.error_code() << " : " << status.error_message() << " : " << status.error_details()
            << std::endl;
  return 0;
}
