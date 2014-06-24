
/*
    
    Future idea: Allow the user to provide a block or some other useful identifier
                 (probably another transaction) to provide a lower bound on this command.
    
*/

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <string.h>
#include <callback.h>

typedef long double Number;
//typedef GoogMap<Hash256, int, Hash256Hasher, Hash256Equal >::Map TxMap;
//typedef GoogMap<Hash256, Number, Hash256Hasher, Hash256Equal >::Map TaintMap;

typedef struct {
    Hash160 addr;
    Number value; // long double
} AddressPair;

typedef struct {
    Hash256 tx;
    uint32_t blockNum;
    uint16_t numInputs;
    uint16_t numOutputs;
    AddressPair *input;
    AddressPair *output;
} TxEntry;

typedef struct {
    uint32_t numTx;
    TxEntry *tx;
} TxLog;

// maintains references to all transactions this account is involved in.
typedef GoogMap<Hash160, TxLog, Hash160Hasher, Hash160Equal >::Map AddressMap; 

static inline void printNumber(
    const Number &x
)
{
    printf("%.32Lf ", x);
}

struct Origin:public Callback
{
    optparse::OptionParser parser;

    double threshold;
    uint128_t txTotal;
    const uint8_t *txHash;
    
    AddressMap addressMap;
    uint256_t endTx;
    
    bool isLastBlock;
    

    Origin()
    {
        parser
            .usage("[transaction hash]")
            .version("")
            .description(
                "compute the origins of the funds transferred in the specified transaction"
            )
            .epilog("")
        ;
    }

    virtual const char                   *name() const         { return "origin"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser; }
    virtual bool                         needTXHash() const    { return true;    }

    virtual int init(
        int argc,
        const char *argv[]
    )
    {
        threshold = 1e-5;
        std::vector<uint256_t> rootHashes;
        isLastBlock = false;

        optparse::Values &values = parser.parse_args(argc, argv);

        auto args = parser.args();
        
        if(rootHashes.size() >= 1) {
            info("computing origin from source transaction starting from the beginning of the blockchain");
            loadHash256List(rootHashes, args[1].c_str());
        } else {
            warning("no TX hashes specified, using the infamous 10K pizza TX");
            const char *defaultTX = "a1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d"; // Expensive pizza
            loadHash256List(rootHashes, defaultTX);
        }
        
        if(rootHashes.size() == 2) {
            info("... specifying a lower bounding transaction is not supported at the moment.");
        }
        
        endTx = rootHashes.front();

        static uint8_t empty[kRIPEMD160ByteSize] = { 0x42 };
        static uint64_t sz = 15 * 1000 * 1000;
        addressMap.setEmptyKey(empty);
        addressMap.resize(sz);

        return 0;
    }

    virtual void wrapup()
    {
        // if analysis hasn't occured yet, either perform it now or warn the user before quitting.
        
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *hash
    )
    {
        // allocate a new TxEntry
    }

    virtual void endTX(
        const uint8_t *p
    )
    {
        // add a reference to this TxEntry to the TxLog of each address listed as
        // an _output_ of the tx. this essentially "reverses" the edges. 
    }

    virtual void edge(
        uint64_t      value,
        const uint8_t *upTXHash,
        uint64_t      outputIndex,
        const uint8_t *outputScript,
        uint64_t      outputScriptSize,
        const uint8_t *downTXHash,
        uint64_t      inputIndex,
        const uint8_t *inputScript,
        uint64_t      inputScriptSize
    )
    {
        // record all of this data in the TxEntry
    }
    
    virtual void endBlock(const Block *b) {
        
        // increment block counter if needed
        // check bool indicating if this is the last block. 
        
        // if so, start analysis
        
        // after analysis, abort.
        
    }
    
};

static Origin taint;


