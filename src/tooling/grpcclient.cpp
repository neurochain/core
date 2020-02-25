#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "messages/service.grpc.pb.h"
#include "messages/Message.hpp"

using namespace neuro;

int main() {
  auto channel = CreateChannel("localhost:8081", grpc::InsecureChannelCredentials());
  auto stub = grpcservice::Block::NewStub(channel);
  grpc::ClientContext ctx;
  messages::Block response;
  auto reader = stub->watch(&ctx, google::protobuf::Empty());
  while(reader->Read(&response)) {
    std::cout << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ": "
              << response.header().height()
              << std::endl;
  }
  auto status = reader->Finish();
  std::cout << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ": "
            << status.ok() << " : " << status.error_code() << " : " << status.error_message() << " : " << status.error_details()
            << std::endl;
  return 0;
}
