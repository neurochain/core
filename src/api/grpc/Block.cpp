#include "Block.hpp"

namespace neuro::api::grpc {

Block::Block(Bot *bot) : Api::Api(bot) {}

Block::gStatus Block::by_id(gServerContext *context,
                            const messages::Hash *request,
                            messages::Block *response) {
  auto block = Api::block(*request);
  response->CopyFrom(block);
  return gStatus::OK;
}

Block::gStatus Block::by_height(gServerContext *context, const gUInt64 *request,
                                messages::Block *response) {
  auto block = Api::block(request->value());
  response->CopyFrom(block);
  return gStatus::OK;
}

Block::gStatus Block::last(gServerContext *context, const gUInt64 *request,
                           messages::Block *response) {
  auto blocks = Api::last_blocks(request->value());
  response->CopyFrom(blocks);
  return gStatus::OK;
}

Block::gStatus Block::total(gServerContext *context, const gEmpty *request,
                            messages::_NCCAmount *response) {
  response->set_value(Api::total_nb_blocks());
  return gStatus::OK;
}

Block::gStatus Block::subscribe(
    gServerContext *context, const gEmpty *request,
    ::grpc::ServerWriter<messages::Blocks> *writer) {
  return gStatus::OK;
}

}  // namespace neuro::api::grpc
