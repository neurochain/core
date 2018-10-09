#include "messages.pb.h"
#include "crypto/Ecc.hpp"
#include "crypto/Hash.hpp"
#include "common/types.hpp"
#include "messages/Message.hpp"
#include "messages/Hasher.hpp"
#include "ledger/LedgerMongodb.hpp"

#include <sys/stat.h>

#include <boost/program_options.hpp>


namespace po = boost::program_options;
namespace neuro
{

inline bool exists_file (const std::string& name)
{
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}


class Wallet
{

    private:
        std::unique_ptr<neuro::crypto::Ecc> ecc_;
        std::unique_ptr<neuro::ledger::LedgerMongodb> ledger_;
        std::vector<neuro::messages::Transaction> trxs_;

        std::unique_ptr<neuro::messages::Hasher> address_;

    public:
        Wallet( const std::string &pathpriv,
                const std::string &pathpub )
        {
            ecc_ = std::make_unique<neuro::crypto::Ecc>(pathpriv,pathpub);
            ledger_ = std::make_unique<neuro::ledger::LedgerMongodb>("mongodb://127.0.0.1:27017/neuro","neuro");

            Buffer key_pub_raw;
            ecc_->mutable_public_key()->save(&key_pub_raw);

            address_ = std::make_unique<neuro::messages::Hasher>(key_pub_raw);
        }

        void getTransactions()
        {
            trxs_.clear();
            neuro::ledger::Ledger::Filter _filter;
            _filter.output_key_id(*address_);


            if (ledger_->for_each(_filter,
                            [&](const neuro::messages::Transaction &a){
                                std::string t;
                                messages::to_json(a, &t);
                                trxs_.push_back(std::move(a));
                                return true;
                            }))
                            {
                                std::cout << "Found " << trxs_.size() << " Trxs" << std::endl;
                            }else
                            {
                                std::cout << "failed Query" << std::endl;
                            }
        }

        void getLastBlockHeigth()
        {

            int32_t lastheight = ledger_->height();
            std::cout << "Height " << lastheight << std::endl;
        }

        void showlastBlock( )
        {
            neuro::messages::Block b;
            if ( ledger_->get_block(ledger_->height(), &b))
            {
                std::string t;
                messages::to_json(b,&t);
                std::cout << t << std::endl;
            }else
            {
                std::cout << "Get Block " << ledger_->height() << " Failed" << std::endl;
            }


        }


        ~Wallet() {}
};


int main(int argc, char *argv[])
{
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "Produce help message.")
    ("keypub,p", po::value<std::string>()->default_value("key.pub"), "File path for public key (.pub)")
    ("key,k", po::value<std::string>()->default_value("key.priv"), "File path for private key (.priv)")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    try
    {
        po::notify(vm);
    }
    catch (po::error &e)
    {
        return 1;
    }

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 1;
    }

    const std::string keypubPath = vm["keypub"].as<std::string>();
    const std::string keyprivPath = vm["key"].as<std::string>();

    if ( !exists_file(keypubPath) || !exists_file(keyprivPath) )
    {
        std::cout << "Pub || Priv Key not found" << std::endl;
        return 1;
    }



    Wallet w(keyprivPath,keypubPath);
    w.getTransactions();
    w.getLastBlockHeigth();
    w.showlastBlock();


    return 0;
}
}
int main(int argc, char *argv[])
{
    return neuro::main(argc, argv);
}
