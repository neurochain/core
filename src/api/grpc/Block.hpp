#ifndef NEURO_SRC_API_GRPC_BLOCK_HPP_
#define NEURO_SRC_API_GRPC_BLOCK_HPP_

#include "api/Api.hpp"
#include "service.grpc.pb.h"

namespace neuro::api::grpc {
class Block : public Api, public grpcservice::Block::Service {
  using gServerContext = ::grpc::ServerContext;
  using gStatus = ::grpc::Status;
  using gEmpty = google::protobuf::Empty;
  using gBool = google::protobuf::BoolValue;
  using gUInt64 = google::protobuf::UInt64Value;

 public:
  explicit Block(Bot* bot);

  gStatus by_id(gServerContext* context, const messages::Hash* request, messages::Block* response);
  gStatus by_height(gServerContext* context, const gUInt64* request, messages::Block* response);
  gStatus last(gServerContext* context, const gUInt64* request, messages::Block* response);
  gStatus total(gServerContext* context, const gEmpty* request, messages::_NCCAmount* response);
  gStatus subscribe(gServerContext* context, const gEmpty* request, ::grpc::ServerWriter<messages::Blocks>* writer);
};
}  //namespace neuro::api::grpc

#endif  // NEURO_SRC_API_GRPC_BLOCK_HPP_
