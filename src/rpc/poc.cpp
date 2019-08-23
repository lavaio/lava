// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "poc.h"
#include "../poc.h"
#include "chainparams.h"
#include "key_io.h"
#include "keystore.h"
#include "sync.h"
#include "util.h"
#include "util/strencodings.h"
#include "wallet/coincontrol.h"
#include "wallet/wallet.h"

#include <algorithm>
#include <queue>
#include <wallet/rpcwallet.h>
#include <ticket.h>
#include <consensus/tx_verify.h>
#include <net.h>
#include <validation.h>

UniValue getAddressPlotId(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            RPCHelpMan{
                "getaddressplotid",
                "\nReturns a plot id.",
                {{"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Your miner address"}},
                RPCResult{
                    "{\n"
                    "  \"plotid\": nnn, (numeric) The plot id\n"
                    "}\n"},
                RPCExamples{
                    HelpExampleCli("getaddressplotid", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"") + HelpExampleRpc("getaddressplotid", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"")},
            }
                .ToString());
    }

    std::vector<std::shared_ptr<CWallet>> wallets = GetWallets();
    auto wallet = wallets.size() == 1 || (request.fHelp && wallets.size() > 0) ? wallets[0] : nullptr;
    if (wallet == nullptr) {
        return NullUniValue;
    }
    CWallet* const pwallet = wallet.get();
    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);
    if (pwallet->IsLocked()) {
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    }
    LOCK(cs_main);
    std::string strAddress = request.params[0].get_str();
    CTxDestination dest = DecodeDestination(strAddress);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address");
    }
    auto keyid = GetKeyForDestination(*pwallet, dest);
    if (keyid.IsNull()) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    }

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("plotid", keyid.GetPlotID());
    return obj;
}

UniValue getMiningInfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            RPCHelpMan{
                "getmininginfo",
                "\nReturns info for poc mining.",
                {},
                RPCResult{
                    "{\n"
                    "  \"height\": nnn\n"
                    "  \"generationSignature\": \"xxx\"\n"
                    "  \"cumulativeDiff\": \"xxx\"\n"
                    "  \"basetarget\": nnn\n"
                    "  \"targetDeadline\": nnn\n"
                    "}\n"},
                RPCExamples{
                    HelpExampleCli("getmininginfo", "") + HelpExampleRpc("getmininginfo", "")},
            }
                .ToString());
    }
    LOCK(cs_main);

    auto height = chainActive.Height() + 1;
    auto diff = chainActive.Tip()->nCumulativeDiff;
    auto block = chainActive.Tip()->GetBlockHeader();
    auto generationSignature = CalcGenerationSignature(block.genSign, block.nPlotID);
    auto nBaseTarget = block.nBaseTarget;
    auto param = Params();
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("height", height);
    obj.pushKV("generationSignature", HexStr<uint256>(generationSignature));
    obj.pushKV("cumulativeDiff", diff.GetHex());
    obj.pushKV("baseTarget", nBaseTarget);
    obj.pushKV("targetDeadline", param.TargetDeadline());
    return obj;
}

UniValue submitNonce(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 4) {
        throw std::runtime_error(
            RPCHelpMan{
                "submitnonce",
                "\nSubmit the nonce form disk.",
                {{"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Your miner address"},
                    {"nonce", RPCArg::Type::STR, RPCArg::Optional::NO, "The nonce you found on disk"},
                    {"deadline", RPCArg::Type::NUM, RPCArg::Optional::NO, "When the next block will be generate"},
                    {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "The block height you want to mine"},},
                RPCResult{
                    "{\n"
                    "  \"accetped\": ture or false\n"
                    "  \"deadline\": \"nnn\"\n"
                    "}\n"},
                RPCExamples{
                    HelpExampleCli("submitnonce", "\"3MhzFQAXQMsmtTmdkciLE3EJsgAQkzR4Sg\" 15032170525642997731 6170762982435 100") + HelpExampleRpc("submitnonce", "\"3MhzFQAXQMsmtTmdkciLE3EJsgAQkzR4Sg\", 15032170525642997731, 6170762982435 100")},
            }
                .ToString());
    }
    std::vector<std::shared_ptr<CWallet>> wallets = GetWallets();
    auto wallet = wallets.size() == 1 || (request.fHelp && wallets.size() > 0) ? wallets[0] : nullptr;
    if (wallet == nullptr) {
        return NullUniValue;
    }
    CWallet* const pwallet = wallet.get();
    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);
    if (pwallet->IsLocked()) {
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    }
    std::string strAddress = request.params[0].get_str();
    CTxDestination dest = DecodeDestination(strAddress);
    if (!IsValidDestination(dest) && dest.type() != typeid(CKeyID)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address");
    }
    auto keyid = boost::get<CKeyID>(dest);
    auto plotID = boost::get<CKeyID>(dest).GetPlotID();
    uint64_t nonce = 0;
    auto nonceStr = request.params[1].get_str();
    if (!ParseUInt64(nonceStr, &nonce)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid nonce");
    }
    uint64_t deadline = request.params[2].get_int64();
    int height = request.params[3].get_int();
    CKey key;
    pwallet->GetKey(keyid, key);
    UniValue obj(UniValue::VOBJ);
    if (blockAssember.UpdateDeadline(height, keyid, nonce, deadline, key)) {
        obj.pushKV("plotid", plotID);
        obj.pushKV("deadline", deadline);
        auto params = Params();
        obj.pushKV("targetdeadline", params.TargetDeadline());
    } else {
        obj.pushKV("accept", false);
    }
    return obj;
}

UniValue getslotinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            RPCHelpMan{ "getslotinfo",
                "Returns an object containing fire stone slot info.\n",
                {
                    {"index", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "slot index."},
                },
                RPCResult{
            "{\n"
            "  \"index\": xx,                  (numeric) the index of fire stone slot \n"
            "  \"price\": xxxxxx,              (numeric) the current price of fire stone slot\n"
            "  \"count\": xx,                  (numeric) the count of tickets in this slot\n"
            "  \"locktime\": xxxxx,            (numeric) the end of this slot\n"
            "}\n" },
                RPCExamples{
                    HelpExampleCli("getslotinfo", "") + HelpExampleCli("getslotinfo", "2")
                },
            }.ToString());
    LOCK(cs_main);
    int index = pticketview->SlotIndex();
    if (!request.params[0].isNull()) {
        index = request.params[0].get_int();
    }
    if (index < 0 || index > pticketview->SlotIndex()) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid slot index");
    }
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("index", index);
    obj.pushKV("price", pticketview->TicketPriceInSlot(index));
    obj.pushKV("count", (uint64_t)pticketview->GetTicketsBySlotIndex(index).size());
    obj.pushKV("locktime", pticketview->LockTime(index));
    return obj;
}

UniValue setfsowner(const JSONRPCRequest& request){
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();

    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
        RPCHelpMan{
            "setfsowner",
            "\nset the mining fs user.\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address to use fs(only keyid)."},
        },
        RPCResult{
            "true|false        (boolean) Returns true if successful\n"
        },
        RPCExamples{
            HelpExampleCli("setfsowner", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\"")},
        }
    .ToString());

    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);
    if (pwallet->IsLocked()) {
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    }

    CTxDestination dest = DecodeDestination(request.params[0].get_str());
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sfSource address");
    }
    if (dest.type() != typeid(CKeyID)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Only support PUBKEYHASH");
    }
    auto keyID = boost::get<CKeyID>(dest);
    CKey fsSourceKey;
    pwallet->GetKey(keyID, fsSourceKey);
    blockAssember.SetFirestoneAt(fsSourceKey);
    return true;
}

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    { "poc",               "getmininginfo",           &getMiningInfo,          {} },
    { "poc",               "submitnonce",             &submitNonce,            {"address", "nonce", "deadline"} },
	{ "poc",               "getaddressplotid",        &getAddressPlotId,       {"address"} },
    { "poc",               "getslotinfo",             &getslotinfo,            {"index"} },
    { "wallet",            "setfsowner",             &setfsowner,            {"address"} },    
};

void RegisterPocRPCCommands(CRPCTable& t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
