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
  using Api::subscribe;

 private:
  // TODO use boost::circular_buffer or limit queue size
  std::queue<messages::Block> _new_block_queue;
  std::condition_variable _is_queue_empty;
  std::mutex _cv_mutex;
  bool _has_subscriber = false;

  void handle_new_block(const messages::Header& header,
                        const messages::Body& body);

 public:
  explicit Block(Bot* bot);

  gStatus by_id(gServerContext* context, const messages::Hash* request, messages::Block* response);
  gStatus by_height(gServerContext* context, const gUInt64* request, messages::Block* response);
  gStatus last(gServerContext* context, const gUInt64* request, messages::Block* response);
  gStatus total(gServerContext* context, const gEmpty* request, messages::_NCCAmount* response);
  gStatus subscribe(gServerContext* context, const gEmpty* request, ::grpc::ServerWriter<messages::Block>* writer);
};
}  //namespace neuro::api::grpc

#endif  // NEURO_SRC_API_GRPC_BLOCK_HPP_
