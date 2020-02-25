#include "Block.hpp"

namespace neuro::api::grpc {

Block::Block(Bot *bot) : Api::Api(bot) {
  Api::subscribe(messages::Type::kBlock, [this](const messages::Header &header,
                                                const messages::Body &body) {
    _block_watcher.handle_new_element(body.block());
  });
}

Block::Status Block::by_id(ServerContext *context,
                            const messages::Hash *request,
                            messages::Block *response) {
  auto block = Api::block(*request);
  response->Swap(&block);
  return Status::OK;
}

Block::Status Block::by_height(ServerContext *context, const UInt64 *request,
                                messages::Block *response) {
  auto block = Api::block(request->value());
  response->Swap(&block);
  return Status::OK;
}

Block::Status Block::lasts(ServerContext *context, const UInt64 *request,
                           messages::Blocks *response) {
  auto blocks = Api::last_blocks(request->value());
  response->Swap(&blocks);
  return Status::OK;
}

Block::Status Block::total(ServerContext *context, const Empty *request,
                            messages::_NCCAmount *response) {
  response->set_value(Api::total_nb_blocks());
  return Status::OK;
}

Block::Status Block::watch(ServerContext *context, const Empty *request,
                                BlockWriter *writer) {
  _block_watcher.watch([&writer](const std::optional<messages::Block>& last_block) {
    return writer->Write(last_block.value());
  });
  return Status::OK;
}

}  // namespace neuro::api::grpc
