#include "Transaction.hpp"

namespace neuro::api::grpc {

Transaction::Transaction(Bot *bot) : Api::Api(bot) {}

Transaction::gStatus Transaction::by_id(gServerContext *context,
                                        const messages::Hash *request,
                                        messages::Transaction *response) {
  return gStatus::OK;
}

Transaction::gStatus Transaction::list(
    gServerContext *context, const messages::TransactionsFilter *request,
    messages::Transactions *response) {
  return gStatus::OK;
}

Transaction::gStatus Transaction::total(gServerContext *context,
                                        const gEmpty *request,
                                        messages::_NCCAmount *response) {
  return gStatus::OK;
}

Transaction::gStatus Transaction::create(
    gServerContext *context, const messages::CreateTransactionBody *request,
    messages::Transaction *response) {
  return Service::create(context, request, response);
}

Transaction::gStatus Transaction::publish(gServerContext *context,
                                          const messages::Publish *request,
                                          Transaction::gEmpty *response) {
  return gStatus::OK;
}

Transaction::gStatus Transaction::subscribe(
    gServerContext *context, const Transaction::gEmpty *request,
    ::grpc::ServerWriter<messages::Transactions> *writer) {
  return gStatus::OK;
}

}  // namespace neuro::api::grpc
