
/*
    
    Future idea: Allow the user to provide a block or some other useful identifier
                 (probably another transaction) to provide a lower bound on this command.
    
*/

#include <util.h>
#include <common.h>
#include <errlog.h>
#include <string.h>
#include <callback.h>


typedef struct {
    Hash160 addr;
    long double value;
} AddressPair;

typedef struct {
    Hash256 tx;
    uint32_t numInputs;
    uint32_t numOutputs;
    AddressPair *input;
    AddressPair *output;
} TxEntry;

// maintains references to all transactions this account is involved in.
typedef GoogMap<Hash160, std::vector<TxEntry*>, Hash160Hasher, Hash160Equal >::Map AddressMap; 


template<> uint8_t *PagedAllocator<TxEntry>::pool = 0;
template<> uint8_t *PagedAllocator<TxEntry>::poolEnd = 0;
static inline TxEntry *allocTxEntry() { return (TxEntry*)PagedAllocator<TxEntry>::alloc(); }

typedef long double Number;

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
    
    AddressMap addressMap;
    
    Hash256 endTx;
    
    bool isLastBlock;
    
    TxEntry *currentEntry;
    

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
        std::vector<Hash256> rootHashes;
        
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
        

        memcpy(endTx, rootHashes.front(), 32);

        static uint8_t empty[kRIPEMD160ByteSize] = { 0x42 };
        static uint64_t sz = 15 * 1000 * 1000;
        addressMap.setEmptyKey(empty);
        addressMap.resize(sz);

        return 0;
    }

    virtual void wrapup()
    {
        // perform analysis and then abort.
        
    }

    virtual void startTX(
        const uint8_t *p,
        const uint8_t *hash
    )
    {
        currentEntry = allocTxEntry();
        
        memcpy(&(currentEntry->tx), hash, 32);
        
        currentEntry->numInputs = 0;
        currentEntry->numOutputs = 0;
        
        if(unlikely(memcmp(hash, endTx, 32))) {
            isLastBlock = true;
        }
    }

    virtual void endTX(
        const uint8_t *p
    )
    {
        // add a reference to this TxEntry to the TxLog of each address listed as
        // an _output_, or reciever of funds in the tx. this essentially "reverses" the edges. 
        
        AddressPair pair;
        uint32_t size = currentEntry->numOutputs;
        for(uint32_t i = 0; i < size; ++i) {
            pair = currentEntry->output[i];
            addressMap[pair.addr].push_back(currentEntry);
        }
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
        if(unlikely(isLastBlock)) {
            wrapup();
        }
    }
    
};

static Origin taint;


