#include <index/ticketslot.h>
#include <primitives/block.h>
#include <chain.h>
#include <validation.h>

std::unique_ptr<TicketSlot> g_ticket_slot;
constexpr char DB_TICKETPRICE = 'T';

/* DB functions */
class TicketSlot::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    uint64_t GetTicketPrice(const CBlockIndex* pindex);
    
    bool ReadTicketPrice(const uint256 blockhash, uint64_t& price);
    
    bool WriteTicketPrice(const uint256 blockhash, const uint64_t price);
};

TicketSlot::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
	BaseIndex::DB(GetDataDir() / "indexes" / "ticketslot", n_cache_size, f_memory, f_wipe)
{}

BaseIndex::DB& TicketSlot::GetDB() const { return *m_db; }

bool TicketSlot::DB::ReadTicketPrice(const uint256 blockhash, uint64_t& price)
{
    std::pair<uint64_t, int> value;
    auto isRead = Read(std::make_pair(DB_TICKETPRICE, blockhash), value);
    price = value.first;
    return isRead;
}

bool TicketSlot::DB::WriteTicketPrice(const uint256 blockhash, const uint64_t price)
{
    CDBBatch batch(*this);
    batch.Write(std::make_pair(DB_TICKETPRICE, blockhash), price);
    return WriteBatch(batch);
}

uint64_t TicketSlot::DB::GetTicketPrice(const CBlockIndex* pindex)
{
    if (pindex == nullptr || pindex->nHeight < nTicketSlot) return 25 * COIN;

    // nHeight >= nTicketSlot
    const uint32_t slots = pindex->nHeight % nTicketSlot;
    const CBlockIndex* slotBlockIdx = pindex->GetAncestor(pindex->nHeight-slots);

    uint64_t price;
    this->ReadTicketPrice(slotBlockIdx->GetBlockHash(), price);

    return price;
}

/* TicketSlot functions */
TicketSlot::TicketSlot(size_t n_cache_size, bool f_memory, bool f_wipe)
	: m_db(MakeUnique<TicketSlot::DB>(n_cache_size, f_memory, f_wipe))
{}

TicketSlot::~TicketSlot()
{
}

bool TicketSlot::Init()
{
    return BaseIndex::Init();
}

bool TicketSlot::WriteBlock(const CBlock& block, const CBlockIndex* pindex)
{
    if (pindex == nullptr || pindex->nHeight < nTicketSlot || pindex->nHeight % nTicketSlot != 0) return true;

    // get last slot price
    auto price = GetTicketPrice(pindex->pprev);
    
    auto ticketList = g_ticketindex->ListTickets(pindex, nTicketSlot);
    uint32_t sumTickets = ticketList.size();
    uint32_t ticketMargin = nTicketSlot * 0.1;

    if (sumTickets > nTicketSlot + ticketMargin) price *= 1.05f;
    else if (sumTickets < nTicketSlot - ticketMargin) price *= 0.95f;

    return m_db->WriteTicketPrice(pindex->GetBlockHash(), price);
}

bool TicketSlot::ReadTicketPrice(const uint256 blockhash, uint64_t& price)
{
    return m_db->ReadTicketPrice(blockhash, price);
}

bool TicketSlot::WriteTicketPrice(const uint256 blockhash, const uint64_t price)
{
    return m_db->WriteTicketPrice(blockhash, price);
}

uint64_t TicketSlot::GetTicketPrice(const CBlockIndex* pindex)
{
    return m_db->GetTicketPrice(pindex);
}