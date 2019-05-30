#ifndef BITCOIN_ASSEMBER_H
#define BITCOIN_ASSEMBER_H

#include "script/script.h"
#include "chainparams.h"
#include "script/script.h"
#include "uint256.h"
#include <stdint.h>

class CPOCBlockAssember
{
public:
    CPOCBlockAssember();

    bool UpdateDeadline(const uint64_t plotID, const uint64_t nonce, const uint64_t deadline, CScript& script);

    void CreateNewBlock(const CScript& scriptPubKeyIn);

private:
    uint256 genSig;
    uint64_t plotID;
    uint64_t nonce;
    uint64_t deadline;
};
#endif // BITCOIN_ASSEMBER_H