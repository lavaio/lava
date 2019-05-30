#ifndef COMMON_POC_H
#define COMMON_POC_H

#include "uint256.h"
#include <string>

using namespace std;

uint256 CalcGenerationSignature(uint256 lastSig, uint64_t lastPlotID);

uint64_t CalcDeadline(const uint256 genSig, const uint64_t height, const uint64_t plotID, const uint64_t nonce);

bool CheckProofOfCapacity(const uint256 genSig, const uint64_t height, const uint64_t plotID, const uint64_t nonce, const uint64_t baseTarget, const uint64_t deadline, const uint64_t targetDeadline);

#endif // end COMMON_POC_H