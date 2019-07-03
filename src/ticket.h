#ifndef BITCOIN_TICKET_H
#define BITCOIN_TICKET_H

#include <script/script.h>
#include <pubkey.h>

CScript GenerateTicketScript(const CPubKey keyid, const int lockHeight);

bool GetPublicKeyFromScript(const CScript script, CPubKey& pubkey);
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

    CPubKey PublicKey() const;

    bool Invalid() const;

    const uint256& GetHash() const { return hash; }

	template <typename Stream>
	inline void Serialize(Stream& s) const {
		s << txid;
		s << n;
		s << redeemScript;
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
