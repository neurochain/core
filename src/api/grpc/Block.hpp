#ifndef NEURO_SRC_API_GRPC_BLOCK_HPP_
#define NEURO_SRC_API_GRPC_BLOCK_HPP_

#include "api/Api.hpp"
#include "service.grpc.pb.h"
#include <optional>

namespace neuro::api::grpc {
class Block : public Api, public grpcservice::Block::Service {
  using ServerContext = ::grpc::ServerContext;
  using Status = ::grpc::Status;
  using Empty = google::protobuf::Empty;
  using Bool = google::protobuf::BoolValue;
  using UInt64 = google::protobuf::UInt64Value;
  using BlockWriter = ::grpc::ServerWriter<messages::Block>;

 private:
  std::optional<messages::Block> _last_block = std::nullopt;
  std::condition_variable _has_new_block;
  mutable std::mutex _cv_mutex;
  std::atomic_bool _has_subscriber = false;

  void handle_new_block(const messages::Header& header,
                        const messages::Body& body);

 public:
  explicit Block(Bot* bot);

  Status by_id(ServerContext* context, const messages::Hash* request,
                messages::Block* response);
  Status by_height(ServerContext* context, const UInt64* request,
                    messages::Block* response);
  Status lasts(ServerContext* context, const UInt64* request,
               messages::Blocks* response);
  Status total(ServerContext* context, const Empty* request,
                messages::_NCCAmount* response);
  Status watch(ServerContext* context, const Empty* request,
                    BlockWriter* writer);
};
}  // namespace neuro::api::grpc

#endif  // NEURO_SRC_API_GRPC_BLOCK_HPP_
