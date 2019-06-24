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

    bool UpdateDeadline(const int height, const uint64_t plotID, const uint64_t nonce, const uint64_t deadline, CScript& script);

    void CreateNewBlock(const CScript& scriptPubKeyIn);

    void Interrupt();

    struct AssemberParams AssemberItems();
    void SetAssemberItems(const int height, const uint64_t plotID, const uint64_t nonce, const uint64_t deadline, CScript& script);
    void setNull();

private:
    void checkDeadline();

private:
    boost::atomic<uint256> genSig;
    boost::atomic_int height;
    boost::atomic_uint64_t plotID;
    boost::atomic_uint64_t nonce;
    boost::atomic_uint64_t deadline;
    boost::atomic<CScript> scriptPubKeyIn;
    std::shared_ptr<CScheduler> scheduler;
    std::shared_ptr<boost::thread> thread;
};

struct AssemberParams {
    int height;
    uint64_t plotID;
    uint64_t nonce;
    uint64_t deadline;
    CScript scriptPubKeyIn;
};

#endif // BITCOIN_ASSEMBER_H