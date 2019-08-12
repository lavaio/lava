#ifndef BITCOIN_TICKET_H
#define BITCOIN_TICKET_H

#include <config/bitcoin-config.h>
#include <script/script.h>
#include <pubkey.h>
#include <amount.h>
#include <dbwrapper.h>

#include <functional>

CScript GenerateTicketScript(const CKeyID keyid, const int lockHeight);

bool DecodeTicketScript(const CScript redeemScript, CKeyID& keyID, int &lockHeight);

bool GetPublicKeyFromScript(const CScript script, CPubKey& pubkey);

bool GetRedeemFromScript(const CScript script, CScript& redeemscript);

class COutPoint;
class CTicket {
public:
    static const int32_t VERSION = 1;

    enum CTicketState {
        IMMATURATE = 0,
        USEABLE,
        OVERDUE,
        UNKNOW
    };

    COutPoint* out;
    CAmount nValue;
    CScript redeemScript;
    CScript scriptPubkey;

    CTicket(const COutPoint& out, const CAmount nValue, const CScript& redeemScript, const CScript &scriptPubkey);

    CTicket() = default;

    ~CTicket();

    CTicketState State(int activeHeight) const;

    int LockTime()const;

    CKeyID KeyID() const;

    bool Invalid() const;

    //void setValue(uint256 ticketid, uint256 txid, uint32_t n, CAmount nValue, CScript redeemscript, CScript scriptpubkey){
	   // this->hash = ticketid;
	   // this->txid = txid;
	   // this->n = n;
    //this->nValue = nValue;
	   // this->redeemScript = redeemscript;
	   // this->scriptPubkey = scriptpubkey;
    //}
};

typedef std::shared_ptr<const CTicket> CTicketRef;
class CBlock;
typedef std::function<bool(const int, const CTicketRef&)> CheckTicketFunc;

class CTicketView : public CDBWrapper {
public: 
    CTicketView(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    ~CTicketView() = default;

    void ConncetBlock(const int height, const CBlock &blk, CheckTicketFunc checkTicket);

    void DisconnectBlock(const int height, const CBlock &blk);

    CAmount CurrentTicketPrice() const;

    std::vector<CTicketRef> CurrentSlotTicket();

    std::vector<CTicketRef> AvailableTickets();

    std::vector<CTicketRef> FindeTickets(const CKeyID key);

    std::vector<CTicketRef> GetTicketsBySlotIndex(const int slotIndex);

    const int SlotIndex() const { return slotIndex; }

    const int SlotLenght();

    const int LockTime();

    /**
     * Disaster recovery functions
     */
    bool SetSynced();

    bool ResetSynced();

    int IsSynced();

    bool EraseDB();

    bool FlushToDisk();

private:
    void LoadTicketFromTicket();

private:
    std::map<int, std::vector<CTicketRef>> ticketsInSlot;
    std::map<CKeyID, std::vector<CTicketRef>> ticketsInAddr;
    CAmount ticketPrice;
    int slotIndex;
    static CAmount BaseTicketPrice;
    uint256 besthash;
};

#endif
