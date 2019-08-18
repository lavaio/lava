#ifndef LAVA_ASSEMBER_H
#define LAVA_ASSEMBER_H

#include <config/bitcoin-config.h>
#include <scheduler.h>
#include <pubkey.h>
#include <key.h>
#include <arith_uint256.h>
#include <chain.h>

class CPOCBlockAssember
{
public:
    CPOCBlockAssember();

    ~CPOCBlockAssember() = default;

    bool UpdateDeadline(const int height, const CKeyID& keyid, const uint64_t nonce, const uint64_t deadline, const CKey& key);

    void CreateNewBlock();

    void SetNull();

    void CheckDeadline();

    CBlockIndex* GetAssemberBlockIndex();
    
    void ConnectBlock();

private:
    uint256       genSig;
    int           height;
    CKeyID        keyid;
    uint64_t      nonce;
    uint64_t      deadline;
    uint64_t      dl;
    CKey          key;
    arith_uint256 nCumulativeDiff;
    boost::mutex  mtx;
    std::shared_ptr<CScheduler> scheduler;
    CBlockIndex*  prevIndex;
};

#endif // BITCOIN_ASSEMBER_H
