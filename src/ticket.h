#ifndef BITCOIN_TICKET_H
#define BITCOIN_TICKET_H

#include <script/script.h>
#include <pubkey.h>

CScript GenerateTicketScript(const CPubKey keyid, const int lockHeight);

bool GetPublicKeyFromScript(const CScript script, CPubKey& pubkey);

#endif
