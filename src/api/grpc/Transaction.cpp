#include "Transaction.hpp"

namespace neuro::api::grpc {

Transaction::Transaction(Bot *bot) : Api::Api(bot) {
  Api::subscribe(
      messages::Type::kTransaction,
      [this](const messages::Header &header, const messages::Body &body) {
        _transaction_watcher.handle_new_element(body.transaction());
      });
}

Transaction::Status Transaction::by_id(ServerContext *context,
                                        const messages::Hash *request,
                                        messages::Transaction *response) {
  auto transaction = Api::transaction(*request);
  response->Swap(&transaction);
  return Status::OK;
}

Transaction::Status Transaction::list(
    ServerContext *context, const messages::TransactionsFilter *request,
    messages::Transactions *response) {
  // TODO wait issue #247 (merge request !153)
  return Status::OK;
}

Transaction::Status Transaction::total(ServerContext *context,
                                        const Empty *request, UInt64 *response) {
  auto total = Api::total_nb_transactions();
  response->set_value(total);
  return Status::OK;
}

Transaction::Status Transaction::create(
    ServerContext *context, const messages::CreateTransactionBody *request,
    String *response) {
  const auto transaction =
      build_transaction(request->key_pub(), request->outputs(),
                        messages::NCCAmount(request->fee()));
  const auto transaction_opt = messages::to_buffer(transaction);
  if (!transaction_opt) {
    // TODO how grpc handle non OK status ?
    return Status::CANCELLED;
  }
  response->set_value(transaction_opt->to_hex());
  return Status::OK;
}

Transaction::Status Transaction::publish(ServerContext *context,
                                          const messages::Publish *request,
                                          Transaction::Empty *response) {
  // TODO rework that, use rest api for now
  return Status::CANCELLED;
}

Transaction::Status Transaction::watch(
    ServerContext *context, const Transaction::Empty *request,
    TransactionWriter *writer) {
  _transaction_watcher.watch([&writer](const std::optional<messages::Transaction>& last_transaction) {
    return writer->Write(last_transaction.value());
  });
  return Status::OK;
}

}  // namespace neuro::api::grpc
