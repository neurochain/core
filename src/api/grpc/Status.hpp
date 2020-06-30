#ifndef NEURO_SRC_API_GRPC_STATUS_H_
#define NEURO_SRC_API_GRPC_STATUS_H_

#include "api/Api.hpp"
#include "api/Monitoring.hpp"
#include "service.grpc.pb.h"

namespace neuro::api::grpc {

class Status : public Api, public grpcservice::Status::Service {
  using gServerContext = ::grpc::ServerContext;
  using gStatus = ::grpc::Status;
  using gEmpty = google::protobuf::Empty;
  using gBool = google::protobuf::BoolValue;
  using Api::balance;

 private:
  Monitoring _monitor;

 public:
  Status(const messages::config::GRPC &config, Bot *bot);
  gStatus balance(gServerContext *context, const messages::_KeyPub *request,
                  messages::_NCCAmount *response);
  gStatus validate_key(gServerContext *context,
                       const messages::_KeyPub *request, gBool *response);
  gStatus ready(gServerContext *context, const gEmpty *request,
                gBool *response);
  gStatus list(gServerContext *context, const gEmpty *request,
               messages::_Peers *response);
  gStatus status(gServerContext *context, const gEmpty *request,
                 messages::Status *response);
};

}  // namespace neuro::api::grpc

#endif  // NEURO_SRC_API_GRPC_STATUS_H_
