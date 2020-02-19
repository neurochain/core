#ifndef NEURO_SRC_API_GRPC_H_
#define NEURO_SRC_API_GRPC_H_

#include "Monitoring.hpp"
#include "api/Api.hpp"
#include "api/Monitoring.hpp"
#include "service.grpc.pb.h"

namespace neuro::api {

class GRPC : public Api, public grpcservice::Status::Service {
  using Api::balance;

 private:
  Monitoring _monitor;

 public:
  GRPC(Bot *bot);
  grpc::Status balance(grpc::ServerContext *context,
                       const messages::_KeyPub *request,
                       messages::_NCCAmount *response);
  grpc::Status validate_key(grpc::ServerContext *context,
                            const messages::_KeyPub *request,
                            google::protobuf::BoolValue *response);
  grpc::Status ready(grpc::ServerContext *context,
                     const ::google::protobuf::Empty *request,
                     google::protobuf::BoolValue *response);
  grpc::Status list(grpc::ServerContext *context,
                    const ::google::protobuf::Empty *request,
                    messages::_Peers *response);
  grpc::Status status(grpc::ServerContext *context,
                      const google::protobuf::Empty *request,
                      messages::Status *response);
};

}  // namespace neuro::api

#endif  // NEURO_SRC_API_GRPC_H_
