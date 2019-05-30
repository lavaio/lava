#include "poc.h"
#include "shabal/shabal.h"

#include <vector>
#include <immintrin.h>

using namespace std;

#define HASH_SIZE 32
#define HASH_CAP 4096
#define SCOOP_SIZE 64
#define PLOT_SIZE (HASH_CAP * SCOOP_SIZE) // 4096*64

uint256 CalcGenerationSignature(uint256 lastSig, uint64_t lastPlotID)
{
    vector<unsigned char> signature(lastSig.size() + sizeof(lastPlotID));
    memcpy(&signature[0], lastSig.begin(), lastSig.size());
    unsigned char* vx = (unsigned char*)&lastPlotID;
    for (auto i = 0; i < sizeof(lastPlotID); i++) {
        signature[lastSig.size() + i] = *(vx + 7 - i);
    }
    shabal_context ctx;
    shabal_init(&ctx, 256);
    shabal(&ctx, &signature[0], signature.size());
    vector<unsigned char> res(32);
    shabal_close(&ctx, 0, 0, &res[0]);
    return uint256(res);
}

char* genNonceChunk(const uint64_t plotID, const uint64_t nonce, bool poc2)
{
    shabal_context ctx;
    _mm256_zeroupper();
    char* gendata = new char[16 + PLOT_SIZE];
    //put plotID
    char* xv = (char*)&plotID;
    for (size_t i = 0; i < 8; i++) {
        gendata[PLOT_SIZE + i] = xv[7 - i];
    }
    //put nonce
    xv = (char*)&nonce;
    for (size_t i = 8; i < 16; i++) {
        gendata[PLOT_SIZE + i] = xv[15 - i];
    }
    //
    size_t len = 0;
    for (size_t i = PLOT_SIZE; i > 0; i -= HASH_SIZE) {
        shabal_init(&ctx, 256);
        len = PLOT_SIZE + 16 - i;
        if (len > HASH_CAP) len = HASH_CAP;
        shabal(&ctx, gendata + i, len);
        shabal_close(&ctx, 0, 0, &gendata[i - HASH_SIZE]);
    }
    shabal_init(&ctx, 256);
    shabal(&ctx, gendata, 16 + PLOT_SIZE);
    char* final = new char[32];
    shabal_close(&ctx, 0, 0, final);
    // XOR with final
    for (size_t i = 0; i < PLOT_SIZE; i++) {
        gendata[i] ^= (final[i % HASH_SIZE]);
    }
    delete[] final;
    if (poc2) {
        char* data = new char[PLOT_SIZE];
        for (size_t i = 0; i < PLOT_SIZE; i += HASH_SIZE) {
            if ((i / HASH_SIZE) % 2 == 0) {
                memmove(data + i, gendata + i, HASH_SIZE);
            } else {
                memmove(data + i, gendata + (PLOT_SIZE - i), HASH_SIZE);
            }
        }
        delete[] gendata;
        return data;
    }
    return gendata;
}

uint64_t CalcDeadline(const uint256 genSig, const uint64_t height, const uint64_t plotID, const uint64_t nonce)
{
    vector<uint8_t> scoopGen(40);
    memcpy(&scoopGen[0], genSig.begin(), genSig.size());
    const uint8_t* mov = (uint8_t*)&height;
    scoopGen[32] = mov[7];
    scoopGen[33] = mov[6];
    scoopGen[34] = mov[5];
    scoopGen[35] = mov[4];
    scoopGen[36] = mov[3];
    scoopGen[37] = mov[2];
    scoopGen[38] = mov[1];
    scoopGen[39] = mov[0];
    shabal_context ctx;
    _mm256_zeroupper();
    shabal_init(&ctx, 256);
    shabal(&ctx, &scoopGen[0], 40);
    char genHash[32];
    shabal_close(&ctx, 0, 0, genHash);
    uint32_t scoop = (((unsigned char)genHash[31]) + 256 * (unsigned char)genHash[30]) % 4096;

    auto chunk = genNonceChunk(plotID, nonce, false);
    vector<uint8_t> sig(32 + 64);
    memcpy(&sig[0], genSig.begin(), genSig.size());
    memcpy(&sig[32], &chunk[scoop * 64], sizeof(uint8_t) * 64);
    shabal_init(&ctx, 256);
    shabal(&ctx, &sig[0], 64 + 32);
    vector<uint8_t> res(32);
    shabal_close(&ctx, 0, 0, &res[0]);
    uint64_t* wertung = (uint64_t*)&res[0];
    delete[] chunk;
    return *wertung;
}

bool CheckProofOfCapacity(const uint256 genSig, const uint64_t height, const uint64_t plotID, const uint64_t nonce, const uint64_t baseTarget, const uint64_t deadline, const uint64_t targetDeadline)
{
    auto dl = CalcDeadline(genSig, height, plotID, nonce);
    return (dl == deadline) && (targetDeadline >= dl / baseTarget);
}