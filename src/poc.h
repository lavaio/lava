#ifndef COMMON_POC_H
#define COMMON_POC_H

#include "uint256.h"
#include <string>

using namespace std;

class CBlockHeader;
class CBlockIndex;
class CBlock;

uint256 CalcGenerationSignature(uint256 lastSig, uint64_t lastPlotID);

uint64_t CalcDeadline(const uint256 genSig, const uint64_t height, const uint64_t plotID, const uint64_t nonce);

uint64_t CalcDeadline(const CBlockHeader* block, const CBlockIndex* prevBlock);

bool CheckProofOfCapacity(const uint256 genSig, const uint64_t height, const uint64_t plotID, const uint64_t nonce, const uint64_t baseTarget, const uint64_t deadline, const uint64_t targetDeadline);

void AdjustBaseTarget(const CBlockIndex* prevBlock, CBlock* block);

uint64_t AdjustBaseTarget(const CBlockIndex* prevBlock, const uint32_t nTime);

#endif // end COMMON_POC_H