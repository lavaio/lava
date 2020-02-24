#ifndef COMMON_POC_H
#define COMMON_POC_H

#include "uint256.h"
#include <string>
#include <pubkey.h>

using namespace std;

class CBlockHeader;
class CBlockIndex;
class CBlock;

// for the classic poc2 plotter check.
uint256 CalcGenerationSignaturePoc2(const uint256& lastSig, uint64_t lastPlotID);

uint64_t CalcDeadlinePoc2(const uint256& genSig, const uint64_t height, const uint64_t plotID, const uint64_t nonce);

uint64_t CalcDeadlinePoc2(const CBlockHeader* block, const CBlockIndex* prevBlock);

bool CheckProofOfCapacityPoc2(const uint256& genSig, const uint64_t height, const uint64_t plotID, const uint64_t nonce, const uint64_t baseTarget, const uint64_t deadline, const uint64_t targetDeadline);

// for the poc2.x
uint256 CalcGenerationSignature(const uint256& lastSig, const uint160& publicKeyID);

uint64_t CalcDeadline(const uint256& genSig, const uint64_t height, const uint160& publicKeyID, const uint64_t nonce);

uint64_t CalcDeadline(const CBlockHeader* block, const CBlockIndex* prevBlock);

bool CheckProofOfCapacity(const uint256& genSig, const uint64_t height, const uint160& publicKeyID, const uint64_t nonce, const uint64_t baseTarget, const uint64_t deadline, const uint64_t targetDeadline);

void AdjustBaseTarget(const CBlockIndex* prevBlock, CBlock* block);

uint64_t AdjustBaseTarget(const CBlockIndex* prevBlock, const uint32_t nTime);

#endif // end COMMON_POC_H