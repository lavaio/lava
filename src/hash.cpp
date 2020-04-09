// Copyright (c) 2013-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <crypto/common.h>
#include <crypto/hmac_sha512.h>


inline uint32_t ROTL32(uint32_t x, int8_t r)
{
    return (x << r) | (x >> (32 - r));
}

unsigned int MurmurHash3(unsigned int nHashSeed, const std::vector<unsigned char>& vDataToHash)
{
    // The following is MurmurHash3 (x86_32), see http://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp
    uint32_t h1 = nHashSeed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = vDataToHash.size() / 4;

    //----------
    // body
    const uint8_t* blocks = vDataToHash.data();

    for (int i = 0; i < nblocks; ++i) {
        uint32_t k1 = ReadLE32(blocks + i*4);

        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    //----------
    // tail
    const uint8_t* tail = vDataToHash.data() + nblocks * 4;

    uint32_t k1 = 0;

    switch (vDataToHash.size() & 3) {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = ROTL32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
    }

    //----------
    // finalization
    h1 ^= vDataToHash.size();
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}

void BIP32Hash(const ChainCode &chainCode, unsigned int nChild, unsigned char header, const unsigned char data[32], unsigned char output[64])
{
    unsigned char num[4];
    num[0] = (nChild >> 24) & 0xFF;
    num[1] = (nChild >> 16) & 0xFF;
    num[2] = (nChild >>  8) & 0xFF;
    num[3] = (nChild >>  0) & 0xFF;
    CHMAC_SHA512(chainCode.begin(), chainCode.size()).Write(&header, 1).Write(data, 32).Write(num, 4).Finalize(output);
}

static inline
void MerkleHash_Sha256Midstate(uint256& parent, const uint256& left, const uint256& right) {
    CSHA256().Write(left.begin(), 32).Write(right.begin(), 32).Midstate(parent.begin(), NULL, NULL);
}

uint256 ComputeFastMerkleRoot(const std::vector<uint256>& hashes) {
    uint256 result_hash = uint256();
    if (hashes.size() == 0) return result_hash;

    // inner is an array of eagerly computed subtree hashes, indexed by tree
    // level (0 being the leaves).
    // For example, when count is 25 (11001 in binary), inner[4] is the hash of
    // the first 16 leaves, inner[3] of the next 8 leaves, and inner[0] equal to
    // the last leaf. The other inner entries are undefined.
    //
    // First process all leaves into 'inner' values.
    uint256 inner[32];
    uint32_t count = 0;
    while (count < hashes.size()) {
        uint256 temp_hash = hashes[count];
        count++;
        // For each of the lower bits in count that are 0, do 1 step. Each
        // corresponds to an inner value that existed before processing the
        // current leaf, and each needs a hash to combine it.
        int level;
        for (level = 0; !(count & (((uint32_t)1) << level)); level++) {
            MerkleHash_Sha256Midstate(temp_hash, inner[level], temp_hash);
        }
        // Store the resulting hash at inner position level.
        inner[level] = temp_hash;
    }

    // Do a final 'sweep' over the rightmost branch of the tree to process
    // odd levels, and reduce everything to a single top value.
    // Level is the level (counted from the bottom) up to which we've sweeped.
    //
    // As long as bit number level in count is zero, skip it. It means there
    // is nothing left at this level.
    int level = 0;
    while (!(count & (((uint32_t)1) << level))) {
        level++;
    }
    result_hash = inner[level];

    while (count != (((uint32_t)1) << level)) {
        // If we reach this point, hash is an inner value that is not the top.
        // We combine it with itself (Bitcoin's special rule for odd levels in
        // the tree) to produce a higher level one.

        // Increment count to the value it would have if two entries at this
        // level had existed and propagate the result upwards accordingly.
        count += (((uint32_t)1) << level);
        level++;
        while (!(count & (((uint32_t)1) << level))) {
            MerkleHash_Sha256Midstate(result_hash, inner[level], result_hash);
            level++;
        }
    }
    // Return result.
    return result_hash;
}
