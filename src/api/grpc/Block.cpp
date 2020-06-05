#include "Block.hpp"

namespace neuro::api::grpc {

Block::Block(Bot *bot) : Api::Api(bot) {
  Api::subscribe(
      messages::Type::kPublish,
      [this](const messages::Header &header, const messages::Body &body) {
        for (auto &block_watcher : _block_watchers) {
          block_watcher.handle_new_element(body.publish().block());
        }
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
  for (auto &block_watcher : _block_watchers) {
    if (!block_watcher.has_watcher()) {
      block_watcher.watch(
          [&writer](const std::optional<messages::Block> &last_block) {
            return writer->Write(last_block.value());
          });

      return Status::OK;
    }
  }
  return Status::CANCELLED;
}

}  // namespace neuro::api::grpc
