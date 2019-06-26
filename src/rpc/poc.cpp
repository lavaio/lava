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
#include "validation.h"
#include "wallet/coincontrol.h"
#include "wallet/wallet.h"

#include <algorithm>
#include <queue>
#include <wallet/rpcwallet.h>

/*
static UniValue registerMinerAccount(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            RPCHelpMan{
                "registermineraccount",
                "\nReturns a plot id for poc mining.",
                {{"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Your miner address"}},
                RPCResult{
                    "{\n"
                    "  \"plotid\": nnn, (numeric) The plot id\n"
                    "  \"address\": \"xxx\"\n"
                    "  \"tx\": \"xxx\"\n"
                    "}\n"},
                RPCExamples{
                    HelpExampleCli("registermineraccount", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"") + HelpExampleRpc("registermineraccount", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XX\"")},
            }
                .ToString());
    }
    LOCK(cs_main);

    UniValue obj(UniValue::VOBJ);

    return obj;
}
*/

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

static UniValue listMinerAccounts(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0) {
        throw std::runtime_error(
            RPCHelpMan{
                "listMinerAccounts",
                "\nSubmit the nonce form disk.",
                {{"address", RPCArg::Type::STR, RPCArg::Optional::NO, "Your miner address"},
                    {"nonce", RPCArg::Type::NUM, RPCArg::Optional::NO, "The nonce you found on disk"},
                    {"deadline", RPCArg::Type::NUM, RPCArg::Optional::NO, "When the next block will be generate"}},
                RPCResult{
                    "{\n"
                    "  \"accounts\": ["
                    "  \"plotid\": nnn,             (numeric) The plot id\n"
                    "  \"address\": \"xxx\n"
                    "   ]"
                    "}\n"},
                RPCExamples{
                    HelpExampleCli("listmineraccounts", "") + HelpExampleRpc("listmineraccounts", "")},
            }
                .ToString());
    }
    LOCK(cs_main);

    UniValue obj(UniValue::VOBJ);

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
                    HelpExampleCli("submitnonce", "\"3MhzFQAXQMsmtTmdkciLE3EJsgAQkzR4Sg\" 15032170525642997731 6170762982435") + HelpExampleRpc("submitnonce", "\"3MhzFQAXQMsmtTmdkciLE3EJsgAQkzR4Sg\", 15032170525642997731, 6170762982435")},
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
    std::shared_ptr<CReserveScript> coinbaseScript = std::make_shared<CReserveScript>();
    coinbaseScript->reserveScript = GetScriptForDestination(dest);
    auto plotID = keyid.GetPlotID();
    uint64_t nonce = 0;
    auto nonceStr = request.params[1].get_str();
    if (!ParseUInt64(nonceStr, &nonce)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid nonce");
    }
    uint64_t deadline = request.params[2].get_int64();
    int height = request.params[3].get_int();
    UniValue obj(UniValue::VOBJ);
    if (blockAssember.UpdateDeadline(height, plotID, nonce, deadline, coinbaseScript->reserveScript)) {
        obj.pushKV("plotid", plotID);
        obj.pushKV("deadline", deadline);
        auto params = Params();
        obj.pushKV("targetdeadline", params.TargetDeadline());
    } else {
        obj.pushKV("accept", false);
    }
    return obj;
}

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    //{ "poc",               "registermineraccount",    &registerMinerAccount,   {"address"} },
    { "poc",               "getmininginfo",           &getMiningInfo,          {} },
    { "poc",               "submitnonce",             &submitNonce,            {"address", "nonce", "deadline"} },
	{ "poc",               "getaddressplotid",        &getAddressPlotId,       {"address"} },
};

void RegisterPocRPCCommands(CRPCTable& t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
