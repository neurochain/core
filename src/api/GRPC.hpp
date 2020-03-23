#ifndef NEURO_SRC_API_GRPC_HPP_
#define NEURO_SRC_API_GRPC_HPP_

#include "Api.hpp"
#include "grpc/Block.hpp"
#include "grpc/Status.hpp"
#include "grpc/Transaction.hpp"

namespace neuro::api {

class GRPC : public Api {
 private:
  grpc::Block _block_service;
  grpc::Transaction _transaction_service;
  grpc::Status _status_service;
  std::thread _server_thread;
  std::unique_ptr<::grpc::Server> _server;

 public:
  GRPC(const messages::config::GRPC &config, Bot *bot);
  ~GRPC();
};

}  // namespace neuro::api

#endif  // NEURO_SRC_API_GRPC_HPP_
