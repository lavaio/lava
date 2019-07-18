#ifndef INDEX_TICKETSLOT_H
#define INDEX_TICKETSLOT_H

#include <dbwrapper.h>
#include <index/ticketindex.h>
#include <pubkey.h>
#include <chainparams.h>

class CBlock;
class CBlockIndex;
class TicketIndex;
class CChainParams;

class TicketSlot final: public CDBWrapper
{
public:
    explicit TicketSlot(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    virtual ~TicketSlot();

    bool ConnectBlock(const CBlock& block, CBlockIndex* pindex);

    bool DisconnectBlock(const CBlock& block, CBlockIndex* pindexDelete);

    uint64_t GetTicketPrice(CBlockIndex* pindex);

private:
    float GetTicketPriceRate(CBlockIndex* pindex);
    bool ReadTicketPrice(const uint256 blockhash, float& rate);
    bool WriteTicketPrice(const uint256 blockhash, const float rate);
    bool ReadBestTicket(uint256& blockhash);
    bool WriteBestTicket(const uint256 blockhash);
};

// The global ticket slot
extern std::unique_ptr<TicketSlot> g_ticket_slot;
#endif // !INDEX_TICKETSLOT_H
