#include "Transaction.hpp"

namespace neuro::api::grpc {

Transaction::Transaction(Bot *bot) : Api::Api(bot) {
  Api::subscribe(
      messages::Type::kTransaction,
      [this](const messages::Header &header, const messages::Body &body) {
        this->handle_new_transaction(header, body);
      });
}

Transaction::Status Transaction::by_id(ServerContext *context,
                                        const messages::Hash *request,
                                        messages::Transaction *response) {
  auto transaction = Api::transaction(*request);
  response->CopyFrom(transaction);
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

void Transaction::handle_new_transaction(const messages::Header &header,
                                         const messages::Body &body) {
  if (_has_subscriber) {
    _new_transaction_queue.push(body.transaction());
    _is_queue_empty.notify_all();
  }
}

Transaction::Status Transaction::subscribe(
    ServerContext *context, const Transaction::Empty *request,
    ::grpc::ServerWriter<messages::Transaction> *writer) {
  _has_subscriber = true;
  while (_has_subscriber) {
    std::unique_lock cv_lock(_cv_mutex);
    _is_queue_empty.wait(cv_lock,
                         [this]() { return !_new_transaction_queue.empty(); });
    auto new_transaction = _new_transaction_queue.front();
    _new_transaction_queue.pop();
    _has_subscriber = writer->Write(new_transaction);
  }
  while (!_new_transaction_queue.empty()) {
    _new_transaction_queue.pop();
  }
  return Status::OK;
}

}  // namespace neuro::api::grpc
