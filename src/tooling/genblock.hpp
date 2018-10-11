#include <boost/program_options.hpp>
#include "common/logger.hpp"
#include "common/types.hpp"
#include "crypto/Ecc.hpp"
#include "crypto/Hash.hpp"
#include "ledger/LedgerMongodb.hpp"

#include "messages.pb.h"
#include "messages/Hasher.hpp"
#include "messages/Message.hpp"

#include <optional>
#include <iostream>
#include <fstream>


namespace po = boost::program_options;

namespace neuro {
namespace tooling {
namespace genblock {

bool genblock_from_last_db_block(messages::Block &block,
                                 ledger::LedgerMongodb &ledger,
                                 const uint64_t seed,
                                 std::optional<neuro::messages::KeyPub> author = std::nullopt,
                                 const int32_t last_height = 0,
                                 const int max_trx = 20,
                                 const int max_trail = 5) {

    uint32_t height = last_height;
    if ( height == 0 ){
        height = ledger.height();
    }
    neuro::messages::Block last_block;
    if ( ! ledger.get_block( height, &last_block) ) {
        return false;
    }

    neuro::messages::BlockHeader * header =  block.mutable_header();

    if ( author )
    {
        header->mutable_author()->CopyFrom(*author);
    }
    else{
    header->mutable_author()->CopyFrom(last_block.header().author());
    }

    header->mutable_previous_block_hash()->CopyFrom(last_block.header().id());
    header->mutable_timestamp()->set_data( last_block.header().timestamp().data() + 1);
    header->set_height(last_block.header().height()+1);

    // DO Transaction
    std::srand(seed);
    std::vector< int > used_transaction ;
    int num_transaction = std::max(std::rand(),max_trx);
    int num_last_transaction = last_block.transactions_size();

    for(int i = 0; i <num_transaction; ++i) {
        int how_trial = 0;
        int rinput = std::rand() %  num_last_transaction,
            routput = std::rand() % num_last_transaction;

        while( std::find(used_transaction.begin(), used_transaction.end(), rinput)
                != used_transaction.end()
             ) {
            rinput = std::rand() % num_last_transaction;
            how_trial++;
            if ( how_trial > max_trail )
                break;
        }
        if ( how_trial > max_trail )
            break;
        how_trial = 0 ;
        used_transaction.push_back(rinput);

        while( routput >= last_block.transactions_size()) {
            routput = std::rand() % num_last_transaction;
            how_trial++;
            if ( how_trial > max_trail )
                break;
        }
        if ( how_trial > max_trail )
            break;

        const neuro::messages::Transaction &sender = last_block.transactions(rinput);
        const neuro::messages::Transaction &recevied = last_block.transactions(routput);

        neuro::messages::Transaction *new_trans = block.add_transactions();

        int32_t num_output = 0; // To used rand
        uint64_t total_ncc = std::atol(sender.outputs(num_output).value().value().c_str());
        uint64_t how_ncc =std::max( total_ncc - 1, static_cast<uint64_t>(std::rand()) );
        // Input from sender
        neuro::messages::Input * input = new_trans->add_inputs();
        input->mutable_id()->CopyFrom(sender.id());
        input->set_output_id(num_output);
        input->mutable_block_id()->CopyFrom(last_block.header().id());
        input->set_key_id(0);

        // Output to recevied
        // Output to sender with min 1 ncc
        neuro::messages::Output * output_revevied = new_trans->add_outputs();
        output_revevied->mutable_address()->CopyFrom(recevied.outputs(num_output).address());
        output_revevied->mutable_value()->set_value(std::to_string(how_ncc));

        neuro::messages::Output * output_sender = new_trans->add_outputs();
        output_sender->mutable_address()->CopyFrom(sender.outputs(num_output).address());
        output_sender->mutable_value()->set_value(std::to_string(total_ncc-how_ncc));

        new_trans->mutable_fees()->set_value("0");
    }

    neuro::Buffer bufid;
    neuro::messages::to_buffer(block,&bufid);

    neuro::messages::Hasher blockid( bufid );
    header->mutable_id()->CopyFrom(blockid);

    return true;

}

} // genblock
} // tooling
} // neuro
