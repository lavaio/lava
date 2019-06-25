// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_POC_H
#define BITCOIN_RPC_POC_H

#include "server.h"
#include <univalue.h>

/** Register a new account for mining. */
//UniValue registerMinerAccount(const JSONRPCRequest& request);

/** List all miners in network. */
//UniValue listMinerAccounts(const JSONRPCRequest& request);

/** get mining info */
UniValue getMiningInfo(const JSONRPCRequest& request);

/** get address plot id info */
UniValue getAddressplotid(const JSONRPCRequest& request);
#endif
