// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPCWALLET_H
#define BITCOIN_WALLET_RPCWALLET_H

#include <string>
#include <actiondb.h>
#include <wallet/wallet.h>
#include <interfaces/chain.h>

class CRPCTable;
class CWallet;
class JSONRPCRequest;
class UniValue;
struct PartiallySignedTransaction;
class CTransaction;

void RegisterWalletRPCCommands(CRPCTable &t);

/**
 * Figures out what wallet, if any, to use for a JSONRPCRequest.
 *
 * @param[in] request JSONRPCRequest that wishes to access a wallet
 * @return nullptr if no wallet should be used, or a pointer to the CWallet
 */
std::shared_ptr<CWallet> GetWalletForJSONRPCRequest(const JSONRPCRequest& request);

std::string HelpRequiringPassphrase(CWallet *);
void EnsureWalletIsUnlocked(CWallet *);
bool EnsureWalletIsAvailable(CWallet *, bool avoidException);

UniValue getaddressinfo(const JSONRPCRequest& request);
UniValue signrawtransactionwithwallet(const JSONRPCRequest& request);

uint256 SendAction(CWallet *const pwallet, const CAction& action, const CKey &key, const CTxDestination& destChange);
CTransactionRef SendMoneyWithOpRet(interfaces::Chain::Lock& locked_chain, CWallet * const pwallet, const CTxDestination& address, CAmount nValue, bool fSubtractFeeFromAmount, const CScript& optScritp, const CCoinControl& coin_control, mapValue_t&& mapValue);
CTransactionRef CreateTicketAllSpendTx(CWallet* const pwallet, std::map<uint256,std::pair<int,CScript>> txScriptInputs, std::vector<CTxOut> outs, CTxDestination& dest, CKey& key);

void ImportScript(CWallet* const pwallet, const CScript& script, const std::string& strLabel, bool isRedeemScript) EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet); // in rpcdump.cpp

#endif //BITCOIN_WALLET_RPCWALLET_H
