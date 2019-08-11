#include <ticket.h>
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

CTicketView::CTicketView(size_t nCacheSize, bool fMemory , bool fWipe) 
    :CDBWrapper(GetDataDir() / "ticket" / "index", nCacheSize, fMemory, fWipe),
    ticketPrice(BaseTicketPrice),
    slotIndex(0) 
{
    LoadTicketFromTicket();
}

void CTicketView::LoadTicketFromTicket()
{

}

void CTicketView::FlushToDisk()
{

}