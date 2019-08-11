#include <ticket.h>
#include <validation.h>
#include <core_io.h>
#include <chainparams.h>
#include <script/standard.h>
#include <util/system.h>
#include <primitives/transaction.h>
#include <key.h>

#include <vector>

using namespace std;

CScript GenerateTicketScript(const CKeyID keyid, const int lockHeight)
{
    auto script = CScript() << CScriptNum(lockHeight) << OP_CHECKLOCKTIMEVERIFY << OP_DROP << OP_DUP << OP_HASH160 << ToByteVector(keyid) << OP_EQUALVERIFY << OP_CHECKSIG;
    return std::move(script);
}

bool DecodeTicketScript(const CScript redeemScript, CKeyID& keyID, int &lockHeight)
{
    CScriptBase::const_iterator pc = redeemScript.begin();
    opcodetype opcodeRet;
    vector<unsigned char> vchRet;
    if (redeemScript.GetOp(pc, opcodeRet, vchRet)) {
        lockHeight = CScriptNum(vchRet, true).getint();
        if (redeemScript.GetOp(pc, opcodeRet, vchRet) 
            && redeemScript.GetOp(pc, opcodeRet, vchRet) 
            && redeemScript.GetOp(pc, opcodeRet, vchRet) 
            && redeemScript.GetOp(pc, opcodeRet, vchRet)
            && redeemScript.GetOp(pc, opcodeRet, vchRet)) {
            if (vchRet.size() == 20) {
                keyID = CKeyID(uint160(vchRet));
                return true;
            }
        }
    }
    return false;
}

bool GetPublicKeyFromScript(const CScript script, CPubKey &pubkey)
{
    CScriptBase::const_iterator pc = script.begin();
    opcodetype opcodeRet;
    vector<unsigned char> vchRet;
    if (script.GetOp(pc, opcodeRet, vchRet) && CScriptNum(vchRet,true)> 0) {
        vchRet.clear();
        if (script.GetOp(pc, opcodeRet, vchRet) && opcodeRet == OP_CHECKLOCKTIMEVERIFY) {
            vchRet.clear();
            if (script.GetOp(pc, opcodeRet, vchRet) && opcodeRet == OP_DROP) {
                vchRet.clear();
                if (script.GetOp(pc, opcodeRet, vchRet) && opcodeRet == OP_DUP) {
					vchRet.clear();
					if (script.GetOp(pc, opcodeRet, vchRet) && opcodeRet == OP_HASH160) {
						vchRet.clear();
						if (script.GetOp(pc, opcodeRet, vchRet) && vchRet.size() == 20) {
							if (script.GetOp(pc, opcodeRet, vchRet) && opcodeRet == OP_EQUALVERIFY) {
								vchRet.clear();
								if (script.GetOp(pc, opcodeRet, vchRet) && opcodeRet == OP_CHECKSIG) {
									vchRet.clear();
									return true;
								}
							}
						}
					}
                }
            }
        }
    }
    return false;
}

bool GetRedeemFromScript(const CScript script, CScript& redeemscript)
{
	CScriptBase::const_iterator pc = script.begin();
	opcodetype opcodeRet;
	vector<unsigned char> vchRet;
	if (script.GetOp(pc, opcodeRet, vchRet) && opcodeRet == OP_RETURN) {
		vchRet.clear();
		if (script.GetOp(pc, opcodeRet, vchRet)) {
			vchRet.clear();
			if (script.GetOp(pc, opcodeRet, vchRet)) {
				redeemscript = CScript(vchRet.begin(),vchRet.end());
				return true;
			}
		}
	}
	return false;
}

CTicket::CTicket(const uint256& txid, const uint32_t n, const CAmount nValue, const CScript& redeemScript, const CScript &scriptPubkey)
    :txid(txid), n(n), redeemScript(redeemScript), scriptPubkey(scriptPubkey), nValue(nValue)
{
	CScriptBase::const_iterator pc = scriptPubkey.begin();
	opcodetype opcodeRet;
	vector<unsigned char> vchRet;
	CScriptID scriptID;
	if (scriptPubkey.GetOp(pc, opcodeRet, vchRet) && opcodeRet == OP_HASH160) {
		vchRet.clear();
		if (scriptPubkey.GetOp(pc, opcodeRet, vchRet)) {
			scriptID = CScriptID(uint160(vchRet));
		}
	}
	// check the redeemScript and scriptPubkey, if unmatch throw
	if (scriptID!=CScriptID(redeemScript))
		throw error("error: unmatched redeemScript and scriptPubkey!");

	// generate hash(txid, n, redeemScript)
	hash = ComputeHash();
}

CTicket::CTicketState CTicket::State(int activeHeight) const
{
	int height = LockTime();
	if (height!=0){
		if (height > activeHeight){
			return CTicketState::IMMATURATE;
		}else if(height<=(activeHeight) && (activeHeight)<(height + Params().SlotLength())) {
			return CTicketState::USEABLE;
		}else{
			return CTicketState::OVERDUE;
		}
	}
	return CTicketState::UNKNOW;
}

int CTicket::LockTime() const
{
	CScriptBase::const_iterator pc = redeemScript.begin();
	opcodetype opcodeRet;
	vector<unsigned char> vchRet;
	if (redeemScript.GetOp(pc, opcodeRet, vchRet) && CScriptNum(vchRet,true)> 0) {
		auto height = CScriptNum(vchRet, false).getint();
		return height;
	}
	return 0;
}


CKeyID CTicket::KeyID() const
{
    CKeyID keyID;
    int lockHeight = 0;
    DecodeTicketScript(redeemScript, keyID, lockHeight);
    return keyID;
}

bool CTicket::Invalid() const 
{
	CScriptBase::const_iterator pc = redeemScript.begin();
	opcodetype opcodeRet;
	vector<unsigned char> vchRet;
	if (redeemScript.GetOp(pc, opcodeRet, vchRet) && CScriptNum(vchRet,true)> 0) {
		vchRet.clear();
		if (redeemScript.GetOp(pc, opcodeRet, vchRet) && opcodeRet == OP_CHECKLOCKTIMEVERIFY) {
			vchRet.clear();
			if (redeemScript.GetOp(pc, opcodeRet, vchRet) && opcodeRet == OP_DROP) {
				vchRet.clear();
				if (redeemScript.GetOp(pc, opcodeRet, vchRet) && vchRet.size() == 33) {
					vchRet.clear();
					if (redeemScript.GetOp(pc, opcodeRet, vchRet) && opcodeRet == OP_CHECKSIG) {
						return false;
					}   
				}
			}
		}
	}
	return true;
}

uint256 CTicket::ComputeHash() const
{
	return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}


CAmount CTicketView::BaseTicketPrice = 160 * COIN;
static const char DB_TICKET_SYNCED_KEY = 'S';
static const char DB_TICKET_SLOT_KEY = 'L';
static const char DB_TICKET_ADDR_KEY = 'A';

void CTicketView::ConncetBlock(const int height, const CBlock &blk, CheckTicketFunc checkTicket)
{
    const auto len = Params().SlotLength();
    if (height % len == 0 && height != 0) { //update ticket price
        if (ticketsInSlot[slotIndex].size() > len) {
            ticketPrice *= 1.05;
        } else if (ticketsInSlot[slotIndex].size() < len) {
            ticketPrice *= 0.95;
        }
        slotIndex = int(height/len);
        ticketPrice = std::max(ticketPrice, 1 * COIN);
    }
    for (auto tx : blk.vtx) {        
        if (!tx->IsTicketTx() || !checkTicket(height, tx->Ticket())) {
            //TODO: logging
            continue;
        }
        auto ticket = tx->Ticket();
        ticketsInSlot[slotIndex].emplace_back(ticket);
        ticketsInAddr[ticket->KeyID()].emplace_back(ticket);
    } 
}

void CTicketView::DisconnectBlock(const int height, const CBlock &blk)
{
    const auto len = Params().SlotLength();

    if (height % len == 0 && height != 0) {
        if (ticketsInSlot[slotIndex].size() > len) {
            ticketPrice /= 1.05;
        } else if (ticketsInSlot[slotIndex].size() < len) {
            ticketPrice /= 0.95;
        }
        ticketsInSlot.erase(slotIndex);
        slotIndex = int(height/len);
        ticketPrice = std::max(ticketPrice, 1 * COIN);
    } else if (height == 0) {
        ticketPrice = BaseTicketPrice;
    }

    for (const auto tx : blk.vtx) {
        const auto ticket = tx->Ticket();
        if (ticket == nullptr) continue;
        ticketsInAddr.erase(tx->Ticket()->KeyID());
    } 
}

CAmount CTicketView::CurrentTicketPrice() const
{
    return ticketPrice;
}

std::vector<CTicketRef> CTicketView::CurrentSlotTicket()
{
    return ticketsInSlot[slotIndex];
}

std::vector<CTicketRef> CTicketView::AvailableTickets() 
{
    std::vector<CTicketRef> tickets;
    if (slotIndex - 1 >= 0) {
        tickets = ticketsInSlot[slotIndex - 1];
    }
    return std::move(tickets);
}

std::vector<CTicketRef> CTicketView::GetTicketsBySlotIndex(const int slotIndex) 
{
    return  ticketsInSlot[slotIndex];
}

std::vector<CTicketRef> CTicketView::FindeTickets(const CKeyID key)
{
    return ticketsInAddr[key];
}

const int CTicketView::SlotLenght()
{
    static int slotLenght = Params().SlotLength();
    return slotLenght;
}

const int CTicketView::LockTime()
{
    return (slotIndex + 1) * SlotLenght();
}

CTicketView::CTicketView(size_t nCacheSize, bool fMemory, bool fWipe) 
    :CDBWrapper(GetDataDir() / "ticket" / "index", nCacheSize, fMemory, fWipe),
    ticketPrice(BaseTicketPrice),
    slotIndex(0) 
{
    LoadTicketFromTicket();
}

void CTicketView::LoadTicketFromTicket()
{
    // Load info from ticket database
    std::pair<char, std::pair<int, uint256>> slotKey;
    std::pair<char, std::pair<CKeyID, uint256>> addrKey;
    std::pair<COutPoint, CScript> value;
    std::unique_ptr<CDBIterator> dbCursor(NewIterator());

    for (dbCursor->SeekToFirst(); dbCursor->Valid(); dbCursor->Next()) {
        // Load ticket slots
        if(dbCursor->GetKey(slotKey) && dbCursor->GetValue(value) && slotKey.first == DB_TICKET_SLOT_KEY) {
            auto vout = value.first;
            auto coin = pcoinsTip->AccessCoin(vout);

            CTicket ticket(vout.hash, vout.n, coin.out.nValue, value.second, coin.out.scriptPubKey);
            CTicketRef ticketref = std::make_shared<CTicket>(ticket);

            ticketsInSlot[slotKey.second.first].push_back(ticketref);
            
        }

        // Load ticket address
        if(dbCursor->GetKey(addrKey) && dbCursor->GetValue(value) && addrKey.first == DB_TICKET_SLOT_KEY) {
            auto vout = value.first;
            auto coin = pcoinsTip->AccessCoin(vout);

            CTicket ticket(vout.hash, vout.n, coin.out.nValue, value.second, coin.out.scriptPubKey);
            CTicketRef ticketref = std::make_shared<CTicket>(ticket);

            ticketsInAddr[addrKey.second.first].push_back(ticketref);
            
        }
    }
    
    // Calculate ticketPrice and slotIndex
    if (!ticketsInSlot.empty()) {
        
        const auto slotLen = Params().SlotLength();
        for (auto it = ticketsInSlot.begin(); it->first > 0 && it != ticketsInSlot.end(); ++ it) {
            if (it->second.size() > slotLen) {
                ticketPrice *= 1.05;
            } else if (it->second.size() < slotLen) {
                ticketPrice *= 0.95;
            }
            slotIndex = it->first;
        }

        ticketPrice = std::max(ticketPrice, 1 * COIN);
    }
}

bool CTicketView::SetSynced()
{
    CDBBatch batch(*this);
    batch.Write(DB_TICKET_SYNCED_KEY, int(1));
    LogPrintf("Ticket DB is SetSynced");
    return WriteBatch(batch);
}

bool CTicketView::ResetSynced()
{
    CDBBatch batch(*this);
    batch.Write(DB_TICKET_SYNCED_KEY, int(0));
    LogPrintf("Ticket DB is ResetSynced");
    return WriteBatch(batch);
}

int CTicketView::IsSynced()
{
    int value;
    if (!Read(DB_TICKET_SYNCED_KEY, value)) {
        return 0;
    }
    return value;
}

bool CTicketView::EraseDB()
{
    string key;
    std::unique_ptr<CDBIterator> dbCursor(NewIterator());

    // Remove all tickets info
    for (dbCursor->SeekToFirst(); dbCursor->Valid(); dbCursor->Next()) {
        if (dbCursor->GetKey(key)) {
            Erase(key);
        }
    }

    SetSynced();
    return true;
}

bool CTicketView::FlushToDisk()
{
    CDBBatch batch(*this);
    // Write ticket slots
    for (auto it = ticketsInSlot.begin(); it != ticketsInSlot.end(); ++ it) {
        for (auto ticketRef: it->second) {
            COutPoint out(ticketRef->GetTxHash(), ticketRef->GetIndex());
            batch.Write(std::make_pair(DB_TICKET_SLOT_KEY, std::make_pair(it->first, ticketRef->GetTxHash())), std::make_pair(out, ticketRef->GetRedeemScript()));
        }
    }

    // Write ticket addrs
    for (auto it = ticketsInAddr.begin(); it != ticketsInAddr.end(); ++ it) {
        for (auto ticketRef: it->second) {
            COutPoint out(ticketRef->GetTxHash(), ticketRef->GetIndex());
            batch.Write(std::make_pair(DB_TICKET_ADDR_KEY, std::make_pair(it->first, ticketRef->GetTxHash())), std::make_pair(out, ticketRef->GetRedeemScript()));
        }
    }
    return WriteBatch(batch);
}