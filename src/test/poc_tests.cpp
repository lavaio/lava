// Copyright (c) 2015-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <poc.h>
#include <random.h>
#include <util/system.h>
#include <test/test_bitcoin.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(poc_tests, BasicTestingSetup)

/* Test initial basetarget */
BOOST_AUTO_TEST_CASE(get_initial_basetarget)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 0; 
    CBlockIndex pindexLast;
    pindexLast.nHeight = 1;
    BOOST_CHECK_EQUAL(AdjustBaseTarget(&pindexLast, nLastRetargetTime), 18325193796L);
}

/* Test basetarget lower than 2700 */
BOOST_AUTO_TEST_CASE(get_middle_basetarget)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 160960;

    std::vector<CBlockIndex> blocks(4);
    for (int i = 0; i < 4; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = 2000 + i;
        blocks[i].nTime = 160000 + 240*i;
        blocks[i].nBaseTarget = 10000;
    }

    BOOST_CHECK_EQUAL(AdjustBaseTarget(&blocks[3], nLastRetargetTime), 10000);
}

/* Test basetarget higher than 2700 */
BOOST_AUTO_TEST_CASE(get_last_basetarget)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    int64_t nLastRetargetTime = 165760;

    std::vector<CBlockIndex> blocks(24);
    for (int i = 0; i < 24; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = 2000 + i;
        blocks[i].nTime = 160000 + 240*i;
        blocks[i].nBaseTarget = 10000;
    }

    BOOST_CHECK_EQUAL(AdjustBaseTarget(&blocks[23], nLastRetargetTime), 10000);
}

//BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
//{
//    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
//    std::vector<CBlockIndex> blocks(10000);
//    for (int i = 0; i < 10000; i++) {
//        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
//        blocks[i].nHeight = i;
//        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
//        blocks[i].nBaseTarget = 10000 + 1000*i;
//    }
//
//    for (int j = 0; j < 1000; j++) {
//        CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
//        CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
//        CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];
//
//        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
//        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
//    }
//}

BOOST_AUTO_TEST_SUITE_END()
