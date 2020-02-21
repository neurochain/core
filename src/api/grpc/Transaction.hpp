#ifndef NEURO_SRC_API_GRPC_TRANSACTION_HPP_
#define NEURO_SRC_API_GRPC_TRANSACTION_HPP_

#include "api/Api.hpp"
#include "service.grpc.pb.h"

namespace neuro::api::grpc {

class Transaction : public Api, public grpcservice::Transaction::Service {
  using gServerContext = ::grpc::ServerContext;
  using gStatus = ::grpc::Status;
  using gEmpty = google::protobuf::Empty;
  using gBool = google::protobuf::BoolValue;
  using gUInt64 = google::protobuf::UInt64Value;
  using gString = google::protobuf::StringValue;
  using Api::subscribe;

 private:
  // TODO use boost::circular_buffer or limit queue size
  std::queue<messages::Transaction> _new_transaction_queue;
  std::condition_variable _is_queue_empty;
  std::mutex _cv_mutex;
  bool _has_subscriber = false;

  void handle_new_transaction(const messages::Header& header,
                              const messages::Body& body);

 public:
  explicit Transaction(Bot* bot);

  gStatus by_id(gServerContext* context, const messages::Hash* request,
                messages::Transaction* response);
  gStatus list(gServerContext* context,
               const messages::TransactionsFilter* request,
               messages::Transactions* response);
  gStatus total(gServerContext* context, const gEmpty* request,
                gUInt64* response);
  gStatus create(gServerContext* context,
                 const messages::CreateTransactionBody* request,
                 gString* response);
  gStatus publish(gServerContext* context, const messages::Publish* request,
                  gEmpty* response);
  gStatus subscribe(gServerContext* context, const gEmpty* request,
                    ::grpc::ServerWriter<messages::Transaction>* writer);
};

}  // namespace neuro::api::grpc

#endif  // NEURO_SRC_API_GRPC_TRANSACTION_HPP_
