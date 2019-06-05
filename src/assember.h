#ifndef BITCOIN_ASSEMBER_H
#define BITCOIN_ASSEMBER_H

#include "chainparams.h"
#include "scheduler.h"
#include "script/script.h"
#include "uint256.h"
#include <boost/atomic.hpp>

#include <stdint.h>

class CPOCBlockAssember
{
public:
    CPOCBlockAssember();

    ~CPOCBlockAssember();

    bool UpdateDeadline(const uint64_t plotID, const uint64_t nonce, const uint64_t deadline, CScript& script);

    void CreateNewBlock(const CScript& scriptPubKeyIn);

    void Interrupt();

private:
    void checkDeadline();

    void setNull();

private:
    boost::atomic<uint256> genSig;
    boost::atomic_uint64_t plotID;
    boost::atomic_uint64_t nonce;
    boost::atomic_uint64_t deadline;
    boost::atomic<CScript> scriptPubKeyIn;
    std::shared_ptr<CScheduler> scheduler;
    std::shared_ptr<boost::thread> thread;
};

#endif // BITCOIN_ASSEMBER_H