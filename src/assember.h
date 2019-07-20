#ifndef BITCOIN_ASSEMBER_H
#define BITCOIN_ASSEMBER_H

#include "chainparams.h"
#include "scheduler.h"
#include "script/script.h"
#include "uint256.h"

#include <stdint.h>

class CPOCBlockAssember
{
public:
    CPOCBlockAssember();

    ~CPOCBlockAssember() = default;

    bool UpdateDeadline(const int height, const uint64_t plotID, const uint64_t nonce, const uint64_t deadline, CScript& script);

    void CreateNewBlock(const CScript& scriptPubKeyIn);

    struct AssemberParams AssemberItems();

    void SetAssemberItems(const int height, const uint64_t plotID, const uint64_t nonce, const uint64_t deadline, CScript& script);

    void setNull();

    void checkDeadline();

private:
	boost::mutex mtx;
    

private:
    uint256 genSig;
    int height;
    uint64_t plotID;
    uint64_t nonce;
    uint64_t deadline;
    CScript scriptPubKeyIn;
};

struct AssemberParams {
    int height;
    uint64_t plotID;
    uint64_t nonce;
    uint64_t deadline;
    CScript scriptPubKeyIn;
};

#endif // BITCOIN_ASSEMBER_H
