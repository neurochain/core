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

 public:
  explicit Transaction(Bot* bot);

  gStatus by_id(gServerContext* context, const messages::Hash* request,
                messages::Transaction* response);
  gStatus list(gServerContext* context,
               const messages::TransactionsFilter* request,
               messages::Transactions* response);
  gStatus total(gServerContext* context, const gEmpty* request,
                messages::_NCCAmount* response);
  gStatus create(gServerContext* context,
                 const messages::CreateTransactionBody* request,
                 messages::Transaction* response);
  gStatus publish(gServerContext* context, const messages::Publish* request,
                  gEmpty* response);
  gStatus subscribe(gServerContext* context, const gEmpty* request,
                    ::grpc::ServerWriter<messages::Transactions>* writer);
};

}  // namespace neuro::api::grpc

#endif  // NEURO_SRC_API_GRPC_TRANSACTION_HPP_
