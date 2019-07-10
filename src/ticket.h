#ifndef BITCOIN_TICKET_H
#define BITCOIN_TICKET_H

#include <script/script.h>
#include <pubkey.h>

CScript GenerateTicketScript(const CKeyID keyid, const int lockHeight);

bool GetKeyIDFromScript(const CScript script, CKeyID& keyid);
bool GetRedeemFromScript(const CScript script, CScript& redeemscript);

class CTicket {
public:
    static const int32_t VERSION = 1;

    enum CTicketState {
        IMMATURATE = 0,
        USEABLE,
        OVERDUE,
		UNKNOW
    };

    CTicket(const uint256& txid, const uint32_t n, const CScript& redeemScript, const CScript &scriptPubkey);

    CTicket() = default;
    ~CTicket() = default;

    CTicketState State(int activeHeight) const;
   
	int LockTime()const;

	CKeyID KeyID() const;

    bool Invalid() const;

    const uint256& GetHash() const { return hash; }
	uint32_t GetIndex() const {return n;}

	template <typename Stream>
	inline void Serialize(Stream& s) const {
		s << txid;
		s << n;
		s << redeemScript;
	}

	template <typename Stream>
	inline void Unserialize(Stream& s) {
		s >> txid;
		s >> n;
		s >> redeemScript;
	}
	
private:
    // only memory
   uint256 hash; //hash(txid, n, redeemScript)
   uint256 txid;
   uint32_t n;
   CScript redeemScript;
   CScript scriptPubkey;
   uint256 ComputeHash() const;
};

typedef std::shared_ptr<const CTicket> CTicketRef;
#endif
