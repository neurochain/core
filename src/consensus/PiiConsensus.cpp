#include "PiiConsensus.hpp"
#include "ledger/Ledger.hpp"

namespace neuro {
namespace consensus {

PiiConsensus::PiiConsensus(std::shared_ptr<ledger::Ledger> ledger,
                           uint32_t block_assembly)
    : _ledger(ledger), _ForkManager(ledger), _valide_block(true) {
  // load all block from
  _assembly_blocks = block_assembly;
  init();
}

void PiiConsensus::init() {
  bool save_valide = _valide_block;
  _valide_block = false;
  _entropies.clear();
  _owner_ordered.clear();
  int height = _ledger->height();
  for (int i = 0; i <= height; ++i) {
    messages::Block block;
    if (_ledger->get_block(i, &block)) {
      add_block(block);
    }
  }
  _valide_block = save_valide;
}

void PiiConsensus::add_block(const neuro::messages::Block &block) {
  neuro::messages::Block last_block, prev_block;
  ///!< compare heigth with ledger
  _ledger->get_block(_ledger->height(), &last_block);
  ///!< verif time
  // auto &last_block_header = last_block.header();
  // int32_t time_of_block =
  //     last_block_header.timestamp().data() -
  //     (last_block_header.timestamp().data() % _block_time) +
  //     (block.header().height() - last_block_header.height()) * _block_time;

  // int32_t time_block =
  //     std::time(nullptr);  //! # T1 used the time of real reception of block
  // block.header().timestamp().data();

  /* if ( (time_of_block > time_block)
           || time_of_block < time_block + _block_time  ) {
       // time_out
       _ledger->fork_add_block(block);
       // don't run for result conflie
       throw std::runtime_error("Owner of time");
       return ;
   }*/
  ///!< verif block suppose Correct Calcul of PII
  if (_valide_block && !check_owner(block.header())) {
    std::cout << "Owner not correct" << std::endl;
    _ledger->fork_add_block(block);
    // possible do this after n blocks 3 of diff of height
    if (_ForkManager.fork_results()) {
      init();
    }
    throw std::runtime_error("Owner not correct");
  }

  ///!< get prev block from this
  _ledger->get_block(block.header().previous_block_hash(), &prev_block);
  if (_valide_block) {
    ForkManager::ForkStatus r = _ForkManager.fork_status(
        block.header(), prev_block.header(), last_block.header());

    if (r == ForkManager::ForkStatus::Non_Fork) {
      _ledger->push_block(block);
    } else {
      /// add it to
      std::cout << "Fork " << std::endl;
      _ledger->fork_add_block(block);
      if (_ForkManager.fork_results()) {
        init();
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
  if (((_last_heigth_block + 1) % _assembly_blocks) == 0) {
    std::cout << "I m in new assembly " << std::endl;
    calcul();
    random_from_hashs();
  }
}  // namespace consensus

void PiiConsensus::add_blocks(
    const std::vector<neuro::messages::Block *> &blocks) {
  for (const auto block : blocks) add_block(*block);
}

void PiiConsensus::random_from_hashs() {
  // Get all hash from 2000 block
  std::vector<std::string> _hash;
  for (uint32_t i = 0; i < _assembly_blocks; i++) {
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
  return operator()(
      ramdon_at((_last_heigth_block + 1) % _assembly_blocks, _nonce_assembly));
}

bool PiiConsensus::check_owner(
    const neuro::messages::BlockHeader &blockheader) const {
  std::string owner_p = operator()(
      ramdon_at(blockheader.height() % _assembly_blocks, _nonce_assembly));
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

void PiiConsensus::show_owner(uint32_t start, uint32_t how) {
  for (uint32_t i = start; i < start + how; ++i) {
    messages::Hash owner_addr;
    owner_addr.ParseFromString(operator()(ramdon_at(i, _nonce_assembly)));

    std::string t;
    messages::to_json(owner_addr, &t);
    std::cout << t << std::endl;
  }
}

}  // namespace consensus
}  // namespace neuro
