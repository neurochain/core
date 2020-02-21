#include "Transaction.hpp"

namespace neuro::api::grpc {

Transaction::Transaction(Bot *bot) : Api::Api(bot) {}

Transaction::gStatus Transaction::by_id(gServerContext *context,
                                        const messages::Hash *request,
                                        messages::Transaction *response) {
  auto transaction = Api::transaction(*request);
  response->CopyFrom(transaction);
  return gStatus::OK;
}

Transaction::gStatus Transaction::list(
    gServerContext *context, const messages::TransactionsFilter *request,
    messages::Transactions *response) {
  // TODO wait issue #247 (merge request !153)
  return gStatus::OK;
}

Transaction::gStatus Transaction::total(gServerContext *context,
                                        const gEmpty *request,
                                        gUInt64 *response) {
  auto total = Api::total_nb_transactions();
  response->set_value(total);
  return gStatus::OK;
}

Transaction::gStatus Transaction::create(
    gServerContext *context, const messages::CreateTransactionBody *request,
    gString *response) {
  const auto transaction = build_transaction(request->key_pub(), request->outputs(),
                                             messages::NCCAmount(request->fee()));
  const auto transaction_opt = messages::to_buffer(transaction);
  if (!transaction_opt) {
    // TODO how grpc handle non OK status ?
    return gStatus::CANCELLED;
  }
  response->set_value(transaction_opt->to_hex());
  return gStatus::OK;
}

Transaction::gStatus Transaction::publish(gServerContext *context,
                                          const messages::Publish *request,
                                          Transaction::gEmpty *response) {
  // TODO rework that, use rest api for now
  return gStatus::CANCELLED;
}

Transaction::gStatus Transaction::subscribe(
    gServerContext *context, const Transaction::gEmpty *request,
    ::grpc::ServerWriter<messages::Transaction> *writer) {
  return gStatus::OK;
}

}  // namespace neuro::api::grpc
