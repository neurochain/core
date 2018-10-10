#include "PiiSus.h"

namespace neuro {
namespace consensus {

PiiSus::PiiSus(ledger::LedgerMongodb &l, uint32_t block_assembly) : _ledger(l),
    _valide_block(true) {
    // cotr
    // load all block from
    _assembly_blocks = block_assembly;
    bool save_valide = _valide_block;
    _valide_block = false;

    int height = _ledger.height();
    for(int i = 0 ; i <= height ; ++i ) {
        messages::Block block;
        _ledger.get_block(i, &block);
        add_block(block);
    }
    _valide_block = save_valide;
}

void PiiSus::add_block(const neuro::messages::Block &b) {
    ///!< verife block Support Correct Calculer of PII
    if (_valide_block && check_owner(b.header())) {
        return;  // TO DO
    }

    neuro::messages::Block last_block, prev_block;
    ///!< compare heigth with ledger
    _ledger.get_block(b.header().previous_block_hash(), &last_block);
    ///!< get prev block from this
    _ledger.get_block(b.header().previous_block_hash(), &prev_block);

    if ( _valide_block ) {
        ForkSus::ForkStatus r = _forksus.fork_status(b.header(), prev_block.header(), last_block.header());
        if ( r == ForkSus::ForkStatus::Non_Fork ) {
            _ledger.push_block(b);
        }
    }


    ///!< build PiiTransaction for intermider calcule
    std::vector<PiiTransaction> _piithx;
    std::vector<std::pair<std::string, uint64_t> > _output, _input;
    ///!< From https://developers.google.com/protocol-buffers/docs/cpptutorial
    for (int j = 0; j < b.transactions_size(); j++) {
        const neuro::messages::Transaction &thx = b.transactions(j);
        _output.clear();
        _input.clear();
        /* output first */
        for (int jo = 0; jo < thx.outputs_size(); jo++) {
            const neuro::messages::Output &output = thx.outputs(jo);
            _output.push_back({output.address().SerializeAsString(),
                               std::atol(output.value().value().c_str())
                              }///!< ProtoBuf to JSON String for uint64_t fix
                             );
        }
        /* inputs */
        double somme_Inputs = 0;

        for (int ji = 0; ji < thx.inputs_size(); ji++) {
            const neuro::messages::Input &input = thx.inputs(ji);
            // Get out ref and block header for time
            // std::unique_ptr<neuro::messages::Transaction> thinput =
            // std::make_unique<neuro::messages::Transaction>();
            messages::Transaction thinput;
            // #error  TO DO get transaction
            uint64_t add = 0;
            auto inputid = input.id();
            if ( _ledger.get_transaction(inputid, &thinput)) {
                const neuro::messages::Output &thxouput = thinput.outputs(input.output_id());
                add = std::atol(thxouput.value().value().c_str());
                _input.push_back({thxouput.address().SerializeAsString(),add});
            }
            somme_Inputs += add;
        }

        for (const auto &input : _input)
            for (const auto &output : _output)
                _piithx.push_back(
                    PiiTransaction{input.first, output.first,
                                   (input.second / somme_Inputs) * output.second, 1});
    }

    addBlocks(_piithx);

    _last_heigth_block = b.header().height();
    if (((_last_heigth_block+1) % _assembly_blocks) == 0) {
        calcul();
        random_from_hashs();
    }
}

void PiiSus::add_blocks(const std::vector<neuro::messages::Block *> &bs) {
    for (const auto block : bs)
        add_block(*block);
}

void PiiSus::random_from_hashs() {
    // Get all hash from 2000 block
    std::vector<std::string> _hash;
    for (uint32_t i = 0; i < _assembly_blocks; i++) {
        neuro::messages::Block b;
        ///!< assembly_blocks/2 is juste pour test
        _ledger.get_block(_last_heigth_block - i - _assembly_blocks / 2, &b);
        _hash.push_back(b.header().id().data());
    }

    _nonce_assembly = 0;
    for (const std::string &h : _hash)
        _nonce_assembly += std::atoi(h.substr(0, 4).c_str());  ///!< just for fun oh
}

uint32_t PiiSus::ramdon_at(int index, uint64_t nonce) const {
    std::srand(nonce);
    int r = std::rand();
    for (int i = 0; i < index; i++)
        r = std::rand();

    return static_cast<uint32_t>(r % _assembly_blocks);
}

std::string PiiSus::get_next_owner() const {
    return operator()(
               ramdon_at((_last_heigth_block + 1) % _assembly_blocks, _nonce_assembly));
}

bool PiiSus::check_owner(const neuro::messages::BlockHeader &bh) const {
    std::string owner_p = operator()(
                              ramdon_at(bh.height() % _assembly_blocks
                                        , _nonce_assembly
                                       ));
    //    #error add here addr from pubkey
    return (bh.author().hex_data() == owner_p);
}

}  // namespace consensus
}  // namespace neuro
