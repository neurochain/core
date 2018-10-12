#ifndef PII_H
#define PII_H


#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <cmath>
#include <algorithm>


namespace neuro
{
namespace consensus
{
///
/// \class pii class
///
class Pii
{
public:

    ///
    /// \struct piiThx "pii.h"
    /// \brief a simple struct of "transaction interaction" between 2 addr
    ///
    struct PiiTransaction
    {
    public:
        std::string from; /*!< from the addr sender*/
        std::string to;  /*!< to the addr recever*/
        double ncc; /*!< ncc the nombre of ncc*/
        uint64_t timencc;/*!< timencc time of ncc*/
    };

    /**
    *   \brief a help structure to store intermediate results
    */
    struct PiiCalculus
    {
    public :
        struct PiiData
        {
            double nbinput, nboutput, mtin, mtout;
            PiiData() : nbinput(0),nboutput(0),mtin(0),mtout(0) {}
        };

        double entropie; ///<! Ei (-1)
        //std::vector<std::tuple<double,double,double>> Erps;
        double sum_inputs, sum_outputs;

        std::unordered_map<std::string, PiiData > entropie_Tij;

        PiiCalculus() : entropie(1), sum_inputs(0), sum_outputs(0) {}
        PiiCalculus(PiiCalculus const& ) = default;

        void update(double new_entropie)
        {
            entropie = new_entropie;
            sum_inputs = sum_outputs = 0;
            entropie_Tij.clear();
        }

        friend bool operator>(const PiiCalculus& l, const PiiCalculus& r)
        {
            return l.entropie > r.entropie;
        }
    };

protected:
    std::unordered_map<std::string, PiiCalculus > _Entropies;
    std::vector< std::string > _owner_ordered;
    int _assembly_owners = 557;
    uint32_t _assembly_blocks;

public:
    /**
    *   \brief
    */
    Pii();

    /**
    * \brief add new piiThx
    * \param ptx the transaction interaction
    */
    void addBlock(const PiiTransaction &piitransaction);

    /**
    *  \brief add more piiThx
    *  \param [in] vpt vector of piiTHx
    */
    void addBlocks(const std::vector<PiiTransaction> &piitransactions);

    /**
    *   \brief Run the calcule of PII ( last step :) )
    */
    void calcul();

    /**
    *   \brief
    */
    std::string operator() (uint32_t index) const;


    void showresultat();
};

} // consensus
} // neuro
#endif // PII_H
