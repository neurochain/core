#include "Status.hpp"

namespace neuro::api::grpc {

Status::Status(const messages::config::GRPC &config, Bot *bot)
    : Api::Api(bot), _monitor(bot) {}

Status::gStatus Status::balance(gServerContext *context,
                           const messages::_KeyPub *request,
                           messages::_NCCAmount *response) {
  const auto balance_amount = Api::balance(*request);
  response->CopyFrom(balance_amount);
  return gStatus::OK;
}

Status::gStatus Status::validate_key(gServerContext *context,
                                const messages::_KeyPub *request,
                                google::protobuf::BoolValue *response) {
  crypto::KeyPub key_pub(*request);
  response->set_value(key_pub.validate());
  return gStatus::OK;
}

Status::gStatus Status::ready(gServerContext *context,
                         const ::google::protobuf::Empty *request,
                         google::protobuf::BoolValue *response) {
  return gStatus::OK;
}

Status::gStatus Status::list(gServerContext *context,
                        const ::google::protobuf::Empty *request,
                        messages::_Peers *response) {
  const auto& peers = Api::peers();
  response->CopyFrom(peers);
  return gStatus::OK;
}

Status::gStatus Status::status(gServerContext *context,
                              const google::protobuf::Empty *request,
                              messages::Status *response) {
  auto status = _monitor.fast_status();
  response->CopyFrom(status);
  return gStatus::OK;
}

}  // namespace neuro::api
