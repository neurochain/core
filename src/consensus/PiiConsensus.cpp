#include "PiiConsensus.hpp"
#include "common/logger.hpp"
#include "ledger/Ledger.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace consensus {

PiiConsensus::PiiConsensus(std::shared_ptr<boost::asio::io_context> io,
                           std::shared_ptr<ledger::Ledger> ledger,
                           std::shared_ptr<networking::Networking> network,
                           int32_t block_assembly)
    : _ledger(ledger),
      _network(network),
      _transaction_pool(_ledger),
      _ForkManager(ledger, _transaction_pool),
      _valide_block(true),
      _timer_of_block_time(*io, boost::asio::chrono::seconds(1)) {
  // load all block from
  _timer_of_block_time.async_wait(
      boost::bind(&PiiConsensus::first_timer_func, this));
  _assembly_blocks = block_assembly;
  init();
}

void PiiConsensus::init() {
  bool save_valide = _valide_block;
  _valide_block = false;
  _entropies.clear();
  _owner_ordered.clear();
  int height = next_height_by_time();  // _ledger->height();

  for (int i = 0; i <= height; ++i) {
    messages::Block block;
    if (_ledger->get_block(i, &block)) {
      add_block(block);
    } else {
      ckeck_run_assembly(i);
    }
  }
  _valide_block = save_valide;
}

int32_t PiiConsensus::next_height_by_time() const {
  messages::Block last_block;
  _ledger->get_block(0, &last_block);
  auto &last_block_header = last_block.header();
  int32_t time_now = std::time(nullptr);

  int32_t height = ((time_now - time_now % BLOCK_PERIODE) -
                    (last_block_header.timestamp().data() -
                     last_block_header.timestamp().data() % BLOCK_PERIODE)) /
                   BLOCK_PERIODE;

  return height;
}

void PiiConsensus::first_timer_func() {
  int32_t time_now = std::time(nullptr);
  int32_t next_time =
      BLOCK_PERIODE -
      time_now % BLOCK_PERIODE;  // time_now - time_now % BLOCK_PERIODE +
                                 // BLOCK_PERIODE;
  _timer_of_block_time.expires_at(_timer_of_block_time.expiry() +
                                  boost::asio::chrono::seconds(next_time));
  _timer_of_block_time.async_wait(boost::bind(&PiiConsensus::timer_func, this));
}

void PiiConsensus::timer_func() {
  build_block();
  int32_t time_now = std::time(nullptr);
  int32_t next_time =
      BLOCK_PERIODE -
      time_now % BLOCK_PERIODE;  // time_now - time_now % BLOCK_PERIODE +
                                 // BLOCK_PERIODE;
  _timer_of_block_time.expires_at(_timer_of_block_time.expiry() +
                                  boost::asio::chrono::seconds(next_time));
  _timer_of_block_time.async_wait(boost::bind(&PiiConsensus::timer_func, this));
}
// TO DO Test
void PiiConsensus::build_block() {
  int32_t next_height = next_height_by_time();
  if (next_height < 1) {
    return;
  }
  std::string next_owner;

  if (next_height < ASSEMBLY_BLOCKS_COUNT) {
    // Fixed 1 assembly block generator
    messages::Hash result;
    messages::from_json(
        "{\"type\": \"SHA256\", \"data\": "
        "\"z5iMSrp79h/zueBTSzitqJW8rNXh4zEdYY96sl3tzkE=\"}",
        &result);
    next_owner = result.SerializeAsString();
  } else {
    next_owner = get_next_owner();
  }
  auto it = _wallets_keys.find(next_owner);
  if (it != _wallets_keys.end()) {
    messages::Block blocks;
    int h = _transaction_pool.build_block(blocks, next_height, it->second.get(),
                                          858993459200lu);
    _transaction_pool.delete_transactions(blocks.transactions());

    LOG_INFO << "Build Block with : " << std::to_string(h) << " transactions";
    add_block(blocks);
    // auto message =
    auto message = std::make_shared<messages::Message>();
    auto header = message->mutable_header();
    messages::fill_header(header);
    message->add_bodies()->mutable_block()->CopyFrom(blocks);
    _network->send(message, networking::ProtocolType::PROTOBUF2);
  }
}

void PiiConsensus::add_transaction(const messages::Transaction &transaction) {
  _transaction_pool.add_transactions(transaction);
}

bool PiiConsensus::block_in_ledger(const messages::Hash &id) {
  messages::Block block;
  if (!_ledger->get_block(id, &block)) {
    if (!_ledger->fork_get_block(id, &block)) {
      return false;
    }
  }
  return true;
}

void PiiConsensus::ckeck_run_assembly(int32_t height) {
  if ((height % _assembly_blocks) == 0) {
    LOG_INFO << "Run PII " << std::to_string(height);
    calcul();
    random_from_hashs();
  }
}

void PiiConsensus::add_block(const neuro::messages::Block &block) {
  neuro::messages::Block last_block, prev_block;
  ///!< fix same id block in the ledger
  /*if (_valide_block && block_in_ledger(block.header().id())) {
    return;
  }*/
  ///!< compare heigth with ledger
  _ledger->get_block(_ledger->height(), &last_block);
  ///!< verif time
  // auto &last_block_header = last_block.header();
  // int32_t time_of_block =
  //     last_block_header.timestamp().data() -
  //     (last_block_header.timestamp().data() % BLOCK_PERIODE) +
  //     (block.header().height() - last_block_header.height()) * BLOCK_PERIODE;

  // int32_t time_block =
  //     std::time(nullptr);  //! # T1 used the time of real reception of block
  // block.header().timestamp().data();

  /* if ( (time_of_block > time_block)
           || time_of_block < time_block + BLOCK_PERIODE  ) {
       // time_out
       _ledger->fork_add_block(block);
       // don't run for result conflie
       throw std::runtime_error("Owner of time");
       return ;
   }*/
  ///!< verif block suppose Correct Calcul of PII
  if (_valide_block && !check_owner(block.header())) {
    _ledger->fork_add_block(block);

    // possible do this after n blocks 3 of diff of height
    if (_ForkManager.fork_results()) {
      init();
      return;
    }
    // throw std::runtime_error("Owner not correct");
  }

  ///!< get prev block from this
  _ledger->get_block(block.header().previous_block_hash(), &prev_block);
  if (_valide_block) {
    ForkManager::ForkStatus r = _ForkManager.fork_status(
        block.header(), prev_block.header(), last_block.header());

    if (r == ForkManager::ForkStatus::Non_Fork) {
      _ledger->push_block(block);
      LOG_INFO << "Consensus : push to Ledger";
    } else {
      /// add it to
      std::cout << "Fork " << std::endl;
      _ledger->fork_add_block(block);
      if (_ForkManager.fork_results()) {
        init();
        return;
      }
    }
  }

  ///!< build Transaction for intermider calcule
  std::vector<Transaction> _piithx;
  std::vector<std::pair<std::string, uint64_t> > _output, _input;

  ///!< From https://developers.google.com/protocol-buffers/docs/cpptutorial
  for (int j = 0; j < block.transactions_size(); j++) {
    const neuro::messages::Transaction &transaction = block.transactions(j);
    _output.clear();
    _input.clear();
    /* output first */
    for (int jo = 0; jo < transaction.outputs_size(); jo++) {
      const neuro::messages::Output &output = transaction.outputs(jo);
      _output.push_back({
          output.address().SerializeAsString(),
          output.value().value()  // std::atol(output.value().value().c_str())
      }                           ///!< ProtoBuf to JSON String for uint64_t fix
      );
    }
    /* inputs */
    double somme_Inputs = 0;

    for (int ji = 0; ji < transaction.inputs_size(); ji++) {
      const neuro::messages::Input &input = transaction.inputs(ji);
      // Get out ref and block header for time
      // std::unique_ptr<neuro::messages::Transaction> thinput =
      // std::make_unique<neuro::messages::Transaction>();
      messages::Transaction thinput;
      // #error  TO DO get transaction
      uint64_t add = 0;
      auto inputid = input.id();

      if (_ledger->get_transaction(inputid, &thinput)) {
        const neuro::messages::Output &thxouput =
            thinput.outputs(input.output_id());
        add = thxouput.value()
                  .value();  // std::atol(thxouput.value().value().c_str());

        _input.push_back({thxouput.address().SerializeAsString(), add});
      } else {
        _input.push_back({transaction.outputs(0).address().SerializeAsString(),
                          transaction.outputs(0).value().value()});
        add = transaction.outputs(0).value().value();
      }
      somme_Inputs += add;
    }

    for (const auto &input : _input) {
      for (const auto &output : _output) {
        _piithx.push_back(
            Transaction{input.first, output.first,
                        (input.second / somme_Inputs) * output.second, 1});
      }
    }
  }

  addBlocks(_piithx);

  _last_heigth_block = block.header().height();
  ckeck_run_assembly(_last_heigth_block + 1);
}  // namespace consensus

void PiiConsensus::add_blocks(
    const std::vector<neuro::messages::Block *> &blocks) {
  for (const auto block : blocks) add_block(*block);
}

void PiiConsensus::random_from_hashs() {
  // Get all hash from 2000 block
  std::vector<std::string> _hash;
  for (int32_t i = 0; i < _assembly_blocks; i++) {
    neuro::messages::Block b;
    ///!< assembly_blocks/2 is juste pour test
    if (_ledger->get_block(_last_heigth_block - i - _assembly_blocks / 2, &b)) {
      _hash.push_back(b.header().id().data());
    }
  }

  _nonce_assembly = 0;
  for (const std::string &h : _hash)
    _nonce_assembly += std::atoi(h.substr(0, 4).c_str());  ///!< just for fun oh
}

uint32_t PiiConsensus::ramdon_at(int index, uint64_t nonce) const {
  std::srand(nonce);
  int r = std::rand();
  for (int i = 0; i < index; i++) {
    r = std::rand();
  }

  return static_cast<uint32_t>(r % _assembly_blocks);
}

Address PiiConsensus::get_next_owner() const {
  int32_t next_height = next_height_by_time();
  if (next_height < 0) {
    next_height = _last_heigth_block + 1;
  }
  return operator()(ramdon_at(next_height % _assembly_blocks, _nonce_assembly));
}

bool PiiConsensus::check_owner(
    const neuro::messages::BlockHeader &blockheader) const {
  std::string owner_p;
  if (blockheader.height() < ASSEMBLY_BLOCKS_COUNT) {
    // Fixed 1 assembly block generator
    messages::Hash result;
    messages::from_json(
        "{\"type\": \"SHA256\", \"data\": "
        "\"z5iMSrp79h/zueBTSzitqJW8rNXh4zEdYY96sl3tzkE=\"}",
        &result);
    owner_p = result.SerializeAsString();

  } else {
    owner_p = operator()(
        ramdon_at(blockheader.height() % _assembly_blocks, _nonce_assembly));
  }

  Buffer buf(blockheader.author().raw_data());
  messages::Address author_addr(buf);

  /** for debug
  std::string t;
  Buffer buftmp(owner_p);
  messages::Hash owner_addr;
  owner_addr.ParseFromString(owner_p);
  messages::to_json(owner_addr,&t);
  std::cout << "Owner -- " << t << std::endl;
  t = "";
  messages::to_json(author_addr, &t);
  std::cout << "Autho -- " << t << std::endl;
  std::cout << "Height -- " << bh.height() << std::endl;
  */
  return (author_addr.SerializeAsString() == owner_p);
}

std::string PiiConsensus::owner_at(int32_t index) {
  return operator()(ramdon_at(index % _assembly_blocks, _nonce_assembly));
}

void PiiConsensus::add_wallet_keys(const std::shared_ptr<crypto::Ecc> wallet) {
  messages::Address addr(wallet->public_key());
  _wallets_keys[addr.SerializeAsString()] = wallet;
}

}  // namespace consensus
}  // namespace neuro
