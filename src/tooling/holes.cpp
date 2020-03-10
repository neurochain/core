#include "consensus/Config.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"

using namespace neuro;

messages::_KeyPub get_block_author(const ledger::LedgerMongodb &l,
                                   const messages::Assembly &assembly,
                                   const messages::TaggedBlock &block) {
  std::mt19937 rng(assembly.seed() + block.block().header().height());
  consensus::Config config;
  auto max_key_pub =
      std::min(assembly.nb_key_pubs(), (int)config.members_per_assembly);
  auto dist = std::uniform_int_distribution(0, max_key_pub - 1);
  auto writer_rank = dist(rng);
  messages::_KeyPub key_pub;
  l.get_block_writer(assembly.id(), writer_rank, &key_pub);
  return key_pub;
}

int main() {
  auto found_hole = 0;
  consensus::Config config;
  ledger::LedgerMongodb l("mongodb://mongo:27017/testnet", "testnet");
  messages::TaggedBlock last_block;
  auto has_last_block = l.get_last_block(&last_block);
  if (has_last_block) {
    messages::Assembly previous_assembly;
    l.get_assembly(last_block.previous_assembly_id(), &previous_assembly);
    for (auto assembly_height = previous_assembly.height();
         assembly_height >= 0; assembly_height--) {
      std::cerr << "looking in assembly " << assembly_height << std::endl;
      messages::Assembly previous_previous_assembly;
      auto has_previous_previous_assembly =
          l.get_assembly(assembly_height - 2, &previous_previous_assembly);
      if (has_previous_previous_assembly) {
        for (auto h = assembly_height * config.blocks_per_assembly;
             h > (assembly_height - 1) * config.blocks_per_assembly; --h) {
          messages::TaggedBlock b;
          auto has_block = l.get_block(h, &b, false);
          if (!has_block || !b.has_block()) {
            auto key_pub = get_block_author(l, previous_previous_assembly, b);
            found_hole++;
          }
        }
      } else {
        std::cerr << "missing assembly " << assembly_height - 2;
      }
    }
  } else {
    std::cerr << "can't get last block header" << std::endl;
  }
  std::cout << "holes : " << found_hole << std::endl;
  return 0;
}
