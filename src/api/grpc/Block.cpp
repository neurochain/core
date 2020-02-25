#include "Block.hpp"

namespace neuro::api::grpc {

Block::Block(Bot *bot) : Api::Api(bot) {
  Api::subscribe(messages::Type::kBlock, [this](const messages::Header &header,
                                                const messages::Body &body) {
    this->handle_new_block(header, body);
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

void Block::handle_new_block(const messages::Header &header,
                             const messages::Body &body) {
  if (_has_subscriber) {
    _new_block_queue.push(body.block());
    _is_queue_empty.notify_all();
  }
}

Block::Status Block::subscribe(ServerContext *context, const Empty *request,
                                ::grpc::ServerWriter<messages::Block> *writer) {
  _has_subscriber = true;
  while (_has_subscriber) {
    std::unique_lock cv_lock(_cv_mutex);
    _is_queue_empty.wait(cv_lock,
                         [this]() { return !_new_block_queue.empty(); });
    auto new_block = _new_block_queue.front();
    _new_block_queue.pop();
    _has_subscriber = writer->Write(new_block);
  }
  while (!_new_block_queue.empty()) {
    _new_block_queue.pop();
  }
  return Status::OK;
}

}  // namespace neuro::api::grpc
