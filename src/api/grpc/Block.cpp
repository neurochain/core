#include "Block.hpp"

namespace neuro::api::grpc {

Block::Block(Bot *bot) : Api::Api(bot) {
  Api::subscribe(messages::Type::kBlock, [this](const messages::Header &header,
                                                const messages::Body &body) {
    this->handle_new_block(header, body);
  });
}

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

void Block::handle_new_block(const messages::Header &header,
                             const messages::Body &body) {
  if (_has_subscriber) {
    messages::Block new_block;
    new_block.CopyFrom(body.block());
    _new_block_queue.push(new_block);
    _is_queue_empty.notify_all();
  }
}

Block::gStatus Block::subscribe(gServerContext *context, const gEmpty *request,
                                ::grpc::ServerWriter<messages::Block> *writer) {
  writer->SendInitialMetadata();
  _has_subscriber = true;
  while (_has_subscriber) {
    {
      std::unique_lock cv_lock(_cv_mutex);
      _is_queue_empty.wait(cv_lock,
                           [this]() { return !_new_block_queue.empty(); });
    }
    auto new_block = _new_block_queue.front();
    _new_block_queue.pop();
    _has_subscriber = writer->Write(new_block);
  }
  while(!_new_block_queue.empty()) {
    _new_block_queue.pop()
  }
  return gStatus::OK;
}

}  // namespace neuro::api::grpc
