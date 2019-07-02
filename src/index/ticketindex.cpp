#include <index/ticketindex.h>

TicketIndex::TicketIndex(size_t n_cache_size, bool f_memory = false, bool f_wipe = false)
    :CDBWrapper(GetDataDir() / "indexes" / "ticketindex", n_cache_size, f_memory, f_wipe)
{
}

TicketIndex::~TicketIndex()
{
}

bool TicketIndex::ConnectBlock(const CBlock& block, const CBlockIndex* pindex)
{
    //key:B + blockhash + tickethash
    //value: isunspent + spentid + n

    //key: T + tickethash
    //value: cticket
    
    return false;
}

bool TicketIndex::DisconnectBlock(const CBlock& block, const CBlockIndex* pindex)
{
    return false;
}

std::vector<CTicketRef> TicketIndex::ListTickets(const CBlockIndex* pindex, const size_t count)
{
    return std::move(std::vector<CTicketRef>());
}

bool TicketIndex::GetTicket(const uint256& ticketId, CTicketRef& ticket)
{
    return false;
}

bool TicketIndex::WriteTicket(const CTicketRef ticket)
{
    return false;
}