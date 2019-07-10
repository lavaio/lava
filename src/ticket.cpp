#include <ticket.h>

#include <vector>
#include <core_io.h>
#include <script/standard.h>
#include <util/system.h>

using namespace std;

CScript GenerateTicketScript(const CKeyID keyid, const int lockHeight)
{
    auto script = CScript() << CScriptNum(lockHeight) << OP_CHECKLOCKTIMEVERIFY << OP_DROP << OP_DUP << OP_HASH160 << ToByteVector(keyid) << OP_EQUALVERIFY << OP_CHECKSIG;
    return std::move(script);
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
                if (script.GetOp(pc, opcodeRet, vchRet) && vchRet.size() == 33) {
                    pubkey = CPubKey(vchRet);
                    return true;
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


CTicket::CTicket(const uint256& txid, const uint32_t n, const CScript& redeemScript, const CScript &scriptPubkey)
    :txid(txid), n(n), redeemScript(redeemScript), scriptPubkey(scriptPubkey)
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
		}else if(height<(activeHeight) && (activeHeight)<(height+100)) {
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

CPubKey CTicket::PublicKey() const
{
	CPubKey pubkey;
	if(GetPublicKeyFromScript(redeemScript,pubkey)){
		return pubkey;
	}
	return CPubKey();
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