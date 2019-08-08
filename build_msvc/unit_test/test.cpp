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
#include "../../src/actiondb.h"

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
    //genesis.nBits = nBits;
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

BOOST_AUTO_TEST_CASE(calc_deadline)
{
    string hex("cbe922a9bcb308e271badc7b30f541131b841c163434899728a3fba68e6d4699");
    auto ba = ParseHex(hex);
    auto genSig = uint256(ba);
    auto deadline = CalcDeadline(genSig, 1, 12645014192979329200, 15032170525642997731);
    BOOST_CHECK_EQUAL(deadline, 6170762982435);
}

BOOST_AUTO_TEST_CASE(test_bind)
{
    SelectParams(CBaseChainParams::MAIN);
    string secret = "KxMXBrCNaNndDprZy3NDkHLnCqhygHjhwUcJeunmuZFbRBwMycn6";
    string toAddr("17VkcJoDJEHyuCKgGyky8CGNnb1kPgbwr4");
    ECC_Start();
    auto key = DecodeSecret(secret);
    auto from = key.GetPubKey().GetID();
    auto destTo = DecodeDestination(toAddr);
    BOOST_ASSERT(destTo.type() == typeid(CKeyID));
    auto to = boost::get<CKeyID>(destTo);
    auto action = MakeBindAction(from, to);
    std::vector<unsigned char> vchSig;
    BOOST_ASSERT(SignAction(action, key, vchSig));
    auto str = HexStr(vchSig);
    ECC_Stop();
}

BOOST_AUTO_TEST_CASE(test_bind2)
{
    SelectParams(CBaseChainParams::MAIN);
    string secret = "KxMXBrCNaNndDprZy3NDkHLnCqhygHjhwUcJeunmuZFbRBwMycn6";
    string toAddr("17VkcJoDJEHyuCKgGyky8CGNnb1kPgbwr4");
    ECC_Start();
    ECCVerifyHandle globalVerifyHandle;
    auto key = DecodeSecret(secret);
    auto from = key.GetPubKey().GetID();
    auto vch = ParseHex("01000000aaf6202d36bdfa33b01c772a6ce7c4dab02fbf7d4740cdeb7d81e3320fc64f888c7b6725da5e56cb1faa045bee3fc486bc60756cc84b0b15c7b48cb6730acabe5d4d7b99e286adfe68166326a5db968d1e49e60cdc75a84022b0a29129861b62539a9a16cc7197c6df");

    auto action = UnserializeAction(vch);
    BOOST_ASSERT(action.type() == typeid(CBindAction));
    auto actionVch = SerializeAction(action);
    CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
    hasher << actionVch;
    std::vector<unsigned char> vchSig(vch.end() - 65, vch.end());
    CPubKey pubkey;
    BOOST_ASSERT(pubkey.RecoverCompact(hasher.GetHash(), vchSig));
    BOOST_ASSERT(from == pubkey.GetID());
    ECC_Stop();
}

BOOST_AUTO_TEST_CASE(test_relation_db) 
{
    ECC_Start();
    std::shared_ptr<CRelationDB> db(new CRelationDB(10240));
    CKey key1;
    key1.MakeNewKey(true);
    auto keyid1 = key1.GetPubKey().GetID();
    CKey key2;
    key2.MakeNewKey(true);
    auto keyid2 = key1.GetPubKey().GetID();
    CKey key3;
    key3.MakeNewKey(true);
    auto keyid3 = key1.GetPubKey().GetID();
    
    //case 1
    auto txid1 = uint256S("074820448352f1d663cfc4e0810b39ce37153533b4c52711aa68904bddda0021");
    BOOST_ASSERT(db->AcceptAction(txid1, MakeBindAction(keyid1, keyid2)));
    BOOST_ASSERT(db->To(keyid1) == keyid2);
    //case 2
    auto txid2 = uint256S("d4a05480fa91eaf105b45b7bd7f197b52bf220e47d2c4203f6594f91f3775bc2");
    BOOST_ASSERT(db->AcceptAction(txid2, MakeBindAction(keyid1, keyid2)));
    BOOST_ASSERT(db->To(keyid1) == keyid2);
    BOOST_ASSERT(db->RollbackAction(txid2));
    BOOST_ASSERT(db->To(keyid1) == keyid2);
    BOOST_ASSERT(db->RollbackAction(txid1));
    BOOST_ASSERT(db->To(keyid1) == CKeyID());
    //case 3
    auto txid3 = uint256S("da35ffcc5d6eeb4f296c66de8ff8fe1c9c112b726da75cc3296deb6ab3c70e63");
    BOOST_ASSERT(db->AcceptAction(txid3, MakeBindAction(keyid1, keyid2)));
    auto txid4 = uint256S("e44699d1b6247ebad3bf57de281a6788503bc258e93d3a8c36f2a72f6822683d");
    BOOST_ASSERT(db->AcceptAction(txid4, CUnbindAction(keyid1)));
    BOOST_ASSERT(db->To(keyid1) == CKeyID());
    //case 4
    auto txid5 = uint256S("e44699d1b6247ebad3bf57de281a6788503bc25111111a8c36f2a72f6822683d");
    BOOST_ASSERT(db->AcceptAction(txid5, CUnbindAction(keyid1)));
    //case 5
    auto txid6 = uint256S("da35ffcc5d6eeb4f296c66de8ffa6788503bc25111111a8c36f2a72f6822683d");
    BOOST_ASSERT(db->AcceptAction(txid6, MakeBindAction(keyid1, keyid2)));
    auto txid7 = uint256S("074820448352eb4f296c66de8ffa6788503bc25111111a8c36f2a72f6822683d");
    BOOST_ASSERT(db->AcceptAction(txid7, MakeBindAction(keyid1, keyid3)));
    BOOST_ASSERT(db->To(keyid1) == keyid3);
    BOOST_ASSERT(db->RollbackAction(txid7));
    BOOST_ASSERT(db->To(keyid1) == keyid2);
    ECC_Stop();
}


BOOST_AUTO_TEST_CASE(test_decode_action) {
    ECCVerifyHandle evh;
    ECC_Start();
    std::string hex_tx("020000000128574dd066abdb89fa8d6930fa1923e1100c7bfd94cf70659c8e8e541650e8ac000000006a47304402207049daa3d10db54640a871caa94eaaff377f20f9ad50e8d5b46ecec1c9e53f4502200b1c8843c529978d4d3368cba9ee70d2df7e043e5ca5f0e36c3fe2c6551d79ea012102da788aa6e547746556a27bee415ff3187df45033e3ef7e79fe8d506047bd6fb8feffffff02007054870e0000001976a91402b6eb3eb965532c942f1c112c585716db12b06388ac0000000000000000706a4c6d0100000002b6eb3eb965532c942f1c112c585716db12b063090e217eff3b75215b5d106ad861d40e0d46af77203a26502f47c21c7210d74855bf691b62ca79cf7fd587ff1ab64b1ff5924fcbbb47d06898b37e3197a1bc82d3d69b1e6a8ac3a90ac382883e439e9d8bd0a9c0bb88060000");
    std::vector<unsigned char> txData(ParseHex(hex_tx));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    CMutableTransaction tx;
    ssData >> tx;
    auto rtx = MakeTransactionRef(tx);
    std::vector<unsigned char> vchSig;
    bool result = false;
    for (auto vout : rtx->vout) {
        if (vout.nValue != 0) continue;
        auto script = vout.scriptPubKey;
        CScriptBase::const_iterator pc = script.begin();
        opcodetype opcodeRet;
        std::vector<unsigned char> vchRet;
        if (!script.GetOp(pc, opcodeRet, vchRet) || opcodeRet != OP_RETURN) {
            continue;
        }
        script.GetOp(pc, opcodeRet, vchRet);
        auto action = UnserializeAction(vchRet);
        if (vchRet.size() < 65) continue;
        vchSig.clear();
        vchSig.insert(vchSig.end(), vchRet.end() - 65, vchRet.end());
        result = VerifyAction(action, vchSig);
    }
    BOOST_ASSERT(result == true);
    ECC_Stop();
}