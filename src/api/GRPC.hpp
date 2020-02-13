#ifndef NEURO_SRC_API_GRPC_H_
#define NEURO_SRC_API_GRPC_H_

#include "Monitoring.hpp"
#include "api/Api.hpp"
#include "api/Monitoring.hpp"
#include "rest.grpc.pb.h"

namespace neuro::api {

class GRPC : public Api, public messages::Monitoring::Service {
 private:
  Monitoring _monitor;

 public:
  GRPC(Bot *bot);
  grpc::Status get_status(grpc::ServerContext *context,
                          const google::protobuf::Empty *request,
                          messages::Status *response);
};

}  // namespace neuro::api

#endif  // NEURO_SRC_API_GRPC_H_
