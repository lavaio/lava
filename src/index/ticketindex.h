#ifndef INDEX_TICKETINDEX_H
#define INDEX_TICKETINDEX_H

#include <dbwrapper.h>
#include <ticket.h>
#include <pubkey.h>

class CBlock;
class CBlockIndex;

class TicketIndex final: public CDBWrapper
{
public:
    explicit TicketIndex(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    virtual ~TicketIndex();

    bool ConnectBlock(const CBlock& block, const CBlockIndex* pindex);

    bool DisconnectBlock(const CBlock& block, const CBlockIndex* pindex);

    std::vector<CTicketRef> ListTickets(const CBlockIndex* pindex, const size_t count);

    bool GetTicket(const uint256& ticketId, CTicketRef& ticket);

private:
    bool WriteTicket(const CTicketRef ticket);
};

#endif // !INDEX_TICKETINDEX_H
