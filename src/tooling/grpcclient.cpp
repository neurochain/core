#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "messages/Message.hpp"
#include "messages/service.grpc.pb.h"

using namespace neuro;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    return 1;
  }
  auto channel = CreateChannel(argv[1], grpc::InsecureChannelCredentials());
  //  auto stub = grpcservice::Block::NewStub(channel);
  //  messages::Block response;
  auto stub = grpcservice::Transaction::NewStub(channel);
  messages::Transaction response;
  grpc::ClientContext ctx;
  auto reader = stub->watch(&ctx, google::protobuf::Empty());
  while (reader->Read(&response)) {
    if (response.outputs().size() > 0) {
      std::cout << response.id() << std::endl;
      for (auto& output : response.outputs()) {
        if (output.has_data()) {
          std::cout << " " << output.data() << std::endl;
        }
      }
      std::cout << std::endl;
    }
  }
  auto status = reader->Finish();
  std::cout << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ": "
            << status.ok() << " : " << status.error_code() << " : "
            << status.error_message() << " : " << status.error_details()
            << std::endl;
  return 0;
}
