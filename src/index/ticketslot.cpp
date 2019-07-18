#include <index/ticketslot.h>
#include <primitives/block.h>
#include <chain.h>
#include <validation.h>

std::unique_ptr<TicketSlot> g_ticket_slot;
constexpr char DB_TICKETPRICE = 'P';
constexpr char DB_TICKETSLOT = 'T';

TicketSlot::TicketSlot(size_t n_cache_size, bool f_memory, bool f_wipe)
    : CDBWrapper(GetDataDir() / "indexes" / "tickeslot", n_cache_size, f_memory, f_wipe)
{
}

TicketSlot::~TicketSlot()
{
}

bool TicketSlot::ConnectBlock(const CBlock& block, CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        return false;
    }
    
    uint256 blockhash;
    ReadBestTicket(blockhash);

    uint64_t prevTicketPrice = 0;
    // pindex->pprev is the tip of the ActiveChain
    // it means that pindex is 1 block higher than ActiveChain.
    if (pindex->GetBlockHash() != blockhash) {
        for (const auto& i : block.vtx) {
            const auto& tx = *i;
            if (tx.IsTicketTx()) {
                // if has ticket tx, then read prevTicketPrice
                if (prevTicketPrice == 0) prevTicketPrice = GetTicketPrice(pindex->pprev);
                CTicketRef ticket;
                try {
                    ticket = tx.Ticket();

                    // check if ticket price is equal to ticket price received
                    const auto value = tx.vout[ticket->GetIndex()].nValue;
                    if (value != prevTicketPrice) {
                        return false;
                    }
                } catch (...) {
                }
            }
        }
    }

    WriteBestTicket(pindex->GetBlockHash());

    return true;
}

uint64_t TicketSlot::GetTicketPrice(CBlockIndex* pindex)
{
    if (pindex == nullptr || pindex->nHeight < nTicketSlot) return 25 * COIN;

    const auto rate = GetTicketPriceRate(pindex);

    const uint32_t slots = pindex->nHeight % nTicketSlot;
    const uint32_t lastSlotHeight = slots != 0 ? slots : nTicketSlot;

    const auto prevTicketPrice = GetTicketPrice(pindex->GetAncestor(pindex->nHeight-lastSlotHeight));
    const uint64_t currTicketPrice = rate * prevTicketPrice;

    return currTicketPrice == 0 ? COIN : currTicketPrice;
}

/**
 * private methods 
 */
float TicketSlot::GetTicketPriceRate(CBlockIndex* pindex)
{
    if (pindex->nHeight % nTicketSlot != 0) return 1;

    float rate;
    ReadTicketPrice(pindex->GetBlockHash(), rate);

    if (rate == 0) {

        auto ticketList = g_ticket->ListTickets(pindex, nTicketSlot);
        uint32_t sumTickets = ticketList.size();
        uint32_t ticketMargin = nTicketSlot * 0.1;

        if (sumTickets > nTicketSlot + ticketMargin) rate = 1.05f;
        else if (sumTickets < nTicketSlot - ticketMargin) rate = 0.95f;
        else rate = 1;

        WriteTicketPrice(pindex->GetBlockHash(), rate);
    }

    return rate;
}

bool TicketSlot::ReadTicketPrice(const uint256 blockhash, float& rate)
{
    std::pair<float, int> value;
    auto isRead = Read(std::make_pair(DB_TICKETPRICE, blockhash), value);
    rate = value.first;
    return isRead;
}

bool TicketSlot::WriteTicketPrice(const uint256 blockhash, const float rate)
{
    CDBBatch batch(*this);
    batch.Write(std::make_pair(DB_TICKETPRICE, blockhash), rate);
    return WriteBatch(batch);
}

bool TicketSlot::ReadBestTicket(uint256& blockhash)
{
    std::pair<uint256, int> value;
    bool isRead = Read(DB_TICKETSLOT, value);
    blockhash = value.first;
    return isRead;
}

// record the best ticket into disk for sync.
bool TicketSlot::WriteBestTicket(const uint256 blockhash)
{
    CDBBatch batch(*this);
    batch.Write(DB_TICKETSLOT, blockhash);
    return WriteBatch(batch);
}
