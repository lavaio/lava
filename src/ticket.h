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

class TicketValue
{
public:
    uint256 hash;
    uint32_t n;
    CAmount nValue;
    CScript redeemScript;
    CScript scriptPubkey;

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        s << hash;
        s << n;
        s << nValue;
        s << redeemScript;
        s << scriptPubkey;
    }

    template <typename Stream>
    inline void Unserialize(Stream& s) {
        s >> hash;
        s >> n;
        s >> nValue;
        s >> redeemScript;
        s >> scriptPubkey;
    }

};

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

    CTicket(const CTicket& other);

    CTicket();

    ~CTicket();

    CTicketState State(int activeHeight) const;

    int LockTime()const;

    CKeyID KeyID() const;

    bool Invalid() const;

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        s << out->hash;
        s << out->n;
        s << nValue;
        s << redeemScript;
        s << scriptPubkey;
    }

    template <typename Stream>
    inline void Unserialize(Stream& s) {
        s >> out->hash;
        s >> out->n;
        s >> nValue;
        s >> redeemScript;
        s >> scriptPubkey;
    }
};

typedef std::shared_ptr<const CTicket> CTicketRef;
class CBlock;
typedef std::function<bool(const int, const CTicketRef&)> CheckTicketFunc;

class CTicketView : public CDBWrapper {
public: 
    CTicketView(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    ~CTicketView() = default;

    void ConnectBlock(const int height, const CBlock &blk, CheckTicketFunc checkTicket);

    void DisconnectBlock(const int height, const CBlock &blk);

    CAmount CurrentTicketPrice() const;

    std::vector<CTicketRef> CurrentSlotTicket();

    std::vector<CTicketRef> AvailableTickets();

    std::vector<CTicketRef> FindeTickets(const CKeyID key);

    std::vector<CTicketRef> GetTicketsBySlotIndex(const int slotIndex);

    const int SlotIndex() const { return slotIndex; }

    const int SlotLength();

    const int LockTime();

    bool LoadTicketFromDisk(const int height);

private:
    bool WriteTicketsToDisk(const int height, const std::vector<CTicket> &tickets);

    void updateTicketPrice(const int height);

private:
    std::map<int, std::vector<CTicketRef>> ticketsInSlot;
    std::map<CKeyID, std::vector<CTicketRef>> ticketsInAddr;
    CAmount ticketPrice;
    int slotIndex;
    static CAmount BaseTicketPrice;
};

#endif
