#ifndef INDEX_TICKETINDEX_H
#define INDEX_TICKETINDEX_H

#include <dbwrapper.h>
#include <ticket.h>
#include <pubkey.h>
#include <chainparams.h>

class CBlock;
class CBlockIndex;

class TicketIndex final: public CDBWrapper
{
public:
    explicit TicketIndex(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    virtual ~TicketIndex();

    bool ConnectBlock(const CBlock& block, CBlockIndex* pindex);

    bool DisconnectBlock(const CBlock& block, CBlockIndex* pindexDelete);

    std::vector<CTicket> ListTickets(CBlockIndex* pindex, size_t count);

    bool GetTicket(const uint256& ticketId, CTicket& ticket);

private:
    bool WriteTicket(const CTicket ticket, const uint256 blockhash);
    bool ReadBestTicket(uint256& blockhash);
    bool WriteBestTicket(const uint256 blockhash);
};

// The global ticket index
extern std::unique_ptr<TicketIndex> g_ticket;
#endif // !INDEX_TICKETINDEX_H
