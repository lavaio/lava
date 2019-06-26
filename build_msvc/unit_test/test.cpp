#define BOOST_TEST_MODULE maintest
#include <boost/test/unit_test.hpp>
#include <string>
#include <immintrin.h>
#include <stdint.h>

#include "../../src/config/bitcoin-config.h"
#include "../../src/poc.h"
#include "../../src/util/strencodings.h"
#include "../../src/txdb.h"
#include "../../src/consensus/merkle.h"
#include "../../src/key_io.h"
#include "../../src/outputtype.h"
#include "../../src/ticket.h"

using namespace std;

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.nBaseTarget = 14660155037;
    return genesis;
}

static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

BOOST_AUTO_TEST_CASE(test_gen_sig)
{
    string hex("b669bf4065a85b7183f423dabdf5d3cca1ddaebbaf2c58739f549c9010170931");
    auto ba = ParseHex(hex);
    auto lastGenSig = uint256(ba);
    uint64_t lastPlotID = 9100919595565594026;
    auto genSig = CalcGenerationSignature(lastGenSig, lastPlotID);
    hex = "6e8ca7bcb03ef5c605c7e3bbeb973d923eaca239a714fc11a7372c234a649709";
    auto ba2 = ParseHex(hex);
    auto target = uint256(ba2);
    BOOST_CHECK_EQUAL(genSig.GetHex(), target.GetHex());
}

BOOST_AUTO_TEST_CASE(genesis_block)
{
    auto genesis = CreateGenesisBlock(1231006505, 2083236893, 0x1d00ffff, 1, 50 * COIN);
    auto hash = genesis.GetHash().ToString();
    auto merkle = genesis.hashMerkleRoot.GetHex();

    //BOOST_CHECK_EQUAL(genesis, uint256S("0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"));

    auto genesis_test = CreateGenesisBlock(1296688602, 414098458, 0x1d00ffff, 1, 50 * COIN);
    hash = genesis_test.GetHash().ToString();

    auto genesis_reg = CreateGenesisBlock(1296688602, 2, 0x207fffff, 1, 50 * COIN);
    hash = genesis_reg.GetHash().ToString();
}

BOOST_AUTO_TEST_CASE(address2plotid)
{
    SelectParams(CBaseChainParams::MAIN);
    string addr("MhzFQAXQMsmtTmdkciLE3EJsgAQkzR4Sg");
    string secret("KwWKWSfxHSZEWi3Nt7wzbem4D2ksgypT5N24LnnsNhz881d84us7");
    ECC_Start();
    auto key = DecodeSecret(secret);
    auto plotID = key.GetPubKey().GetID().GetPlotID();
    
    ECC_Stop();
}

BOOST_AUTO_TEST_CASE(calc_deadline)
{
    string hex("cbe922a9bcb308e271badc7b30f541131b841c163434899728a3fba68e6d4699");
    auto ba = ParseHex(hex);
    auto genSig = uint256(ba);
    auto deadline = CalcDeadline(genSig, 1, 12645014192979329200, 15032170525642997731);
    BOOST_CHECK_EQUAL(deadline, 6170762982435);
}

BOOST_AUTO_TEST_CASE(genesis)
{
    auto script = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f");
    CTxDestination dest;
    ExtractDestination(script, dest);
    auto str = EncodeDestination(dest);
}

BOOST_AUTO_TEST_CASE(get_address)
{
    SelectParams(CBaseChainParams::MAIN);
    string secret = "KzQgt85D2A9qynEz3rbM1uzUVJANm6PrF4mBxrymzPfkjSK5UJPk";
    //string addr("MhzFQAXQMsmtTmdkciLE3EJsgAQkzR4Sg");
    ECC_Start();
    auto key = DecodeSecret(secret);
    auto pubkey = key.GetPubKey();
    CTxDestination dest = GetDestinationForKey(pubkey, OutputType::LEGACY);
    auto addr = EncodeDestination(dest);
    ECC_Stop();
}

BOOST_AUTO_TEST_CASE(test_script)
{
    SelectParams(CBaseChainParams::TESTNET);
    string secret = "cPxCPyn1CJNovHaVF2UqFh7QZACswJQPtk6GgSjwFFESrWepqvvQ";
    ECC_Start();
    auto key = DecodeSecret(secret);
    auto pubkey = key.GetPubKey();
    auto script = CScript() << CScriptNum(220) << OP_CHECKLOCKTIMEVERIFY << OP_DROP << ToByteVector(pubkey) << OP_CHECKSIG;
    CPubKey pub2;
    auto getRet = GetPublicKeyFromScript(script, pub2);
    BOOST_CHECK_EQUAL(pub2==pubkey, true);
    ECC_Stop();
}