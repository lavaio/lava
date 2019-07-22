#ifndef LAVA_ASSEMBER_H
#define LAVA_ASSEMBER_H

#include <config/bitcoin-config.h>
#include <scheduler.h>
#include <pubkey.h>
#include <uint256.h>

class CPOCBlockAssember
{
public:
    CPOCBlockAssember();

    ~CPOCBlockAssember() = default;

    bool UpdateDeadline(const int height, const CKeyID& keyid, const uint64_t nonce, const uint64_t deadline);

    void CreateNewBlock();

    void SetNull();

    void CheckDeadline();

private:
    uint256      genSig;
    int          height;
    CKeyID       keyid;
    uint64_t     nonce;
    uint64_t     deadline;
    uint64_t     dl;
    boost::mutex mtx;
    std::shared_ptr<CScheduler> scheduler;
};

#endif // BITCOIN_ASSEMBER_H
