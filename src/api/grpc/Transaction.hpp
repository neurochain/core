#ifndef NEURO_SRC_API_GRPC_TRANSACTION_HPP_
#define NEURO_SRC_API_GRPC_TRANSACTION_HPP_

#include "api/Api.hpp"
#include "service.grpc.pb.h"

namespace neuro::api::grpc {

class Transaction : public Api, public grpcservice::Transaction::Service {
  using ServerContext = ::grpc::ServerContext;
  using Status = ::grpc::Status;
  using Empty = google::protobuf::Empty;
  using Bool = google::protobuf::BoolValue;
  using UInt64 = google::protobuf::UInt64Value;
  using String = google::protobuf::StringValue;
  using TransactionWriter =
      ::grpc::ServerWriter<messages::Transaction>;
  using Api::subscribe;

 private:
  // TODO use boost::circular_buffer or limit queue size
  std::queue<messages::Transaction> _new_transaction_queue;
  std::condition_variable _is_queue_empty;
  mutable std::mutex _cv_mutex;
  std::atomic_bool _has_subscriber = false;

  void handle_new_transaction(const messages::Header& header,
                              const messages::Body& body);

 public:
  explicit Transaction(Bot* bot);

  Status by_id(ServerContext* context, const messages::Hash* request,
                messages::Transaction* response);
  Status list(ServerContext* context,
               const messages::TransactionsFilter* request,
               messages::Transactions* response);
  Status total(ServerContext* context, const Empty* request, UInt64* response);
  Status create(ServerContext* context,
                 const messages::CreateTransactionBody* request,
                String* response);
  Status publish(ServerContext* context, const messages::Publish* request,
                 Empty* response);
  Status subscribe(ServerContext* context, const Empty* request,
                    TransactionWriter* writer);
};

}  // namespace neuro::api::grpc

#endif  // NEURO_SRC_API_GRPC_TRANSACTION_HPP_
