#include <index/ticketindex.h>
#include <primitives/block.h>
#include <chain.h>
#include <validation.h>

std::unique_ptr<TicketIndex> g_ticket;
constexpr char DB_TICKETINDEX_SPENT = 'B';
constexpr char DB_TICKETINDEX = 'T';
constexpr char DB_TICKETPRICE = 'P';
constexpr uint32_t TICKETPRICE_ADJUST_SLOTS = 2048;

TicketIndex::TicketIndex(size_t n_cache_size, bool f_memory, bool f_wipe)
    : CDBWrapper(GetDataDir() / "indexes" / "ticketindex", n_cache_size, f_memory, f_wipe)
{
}

TicketIndex::~TicketIndex()
{
}

bool TicketIndex::ConnectBlock(const CBlock& block, CBlockIndex* pindex)
{
    if (pindex == nullptr) {
        return false;
    }
    //key:'B' + blockhash + tickethash
    //value: 1

    //key: 'T' + tickethash
    //value: cticket
    uint256 blockhash;
    ReadBestTicket(blockhash);

    const auto prevTicketPrice = ReadPrevTicketPrice(pindex);

    // pindex->pprev is the tip of the ActiveChain
    // it means that pindex is 1 block higher than ActiveChain.
    if (pindex->GetBlockHash() != blockhash) {
        auto thisblockhash = block.GetHash();

        for (const auto& i : block.vtx) {
            const auto& tx = *i;
            if (tx.IsTicketTx()) {
                CTicketRef ticket;
                try {
                    ticket = tx.Ticket();

                    // check if ticket price is equal to ticket price received
                    auto value = tx.vout[ticket->GetIndex()].nValue;
                    if (value != prevTicketPrice) {
                        return false;
                    }
                } catch (...) {
                    continue;
                }
                // write the ticket into DB
                auto isWrited = WriteTicket(*ticket, thisblockhash);
                if (!isWrited)
                    return false;
            }
        }
    }

    WriteTicketPrice(pindex->GetBlockHash(), CalculateTicketPrice(pindex, prevTicketPrice));
    WriteBestTicket(pindex->GetBlockHash());

    return true;
}

// erase the undo_ticket in level DB
// in fact, Erase is allowed to be failed, as tickets are searched by blockhash.
bool TicketIndex::DisconnectBlock(const CBlock& block, CBlockIndex* pindexDelete)
{
    uint256 blockhash;
    ReadBestTicket(blockhash);

    if (pindexDelete->GetBlockHash() != blockhash) {
        for (int i = 0; i < block.vtx.size(); i++) {
            CTransaction tx = *block.vtx[i];
            if (tx.IsTicketTx()) {
                CTicketRef ticket = tx.Ticket();
                bool isErasedB = Erase(std::make_pair(std::make_pair(DB_TICKETINDEX_SPENT, blockhash), ticket->GetHash()), false);
                bool isErasedT = Erase(std::make_pair(DB_TICKETINDEX, ticket->GetHash()), false);
                if (!isErasedB || !isErasedT) {
                    return false;
                }
            }
        }
    }

    WriteBestTicket(pindexDelete->GetBlockHash());

    return true;
}

// this API is used to calculate ticket price.
std::vector<CTicket> TicketIndex::ListTickets(CBlockIndex* pindex, size_t count)
{
    std::vector<CTicket> ticketList;
    CBlockIndex* blockIndex = pindex;

    for (size_t index = 0; index < count; index++) {
        if (blockIndex == nullptr)
            break;
        uint256 blockhash = *blockIndex->phashBlock;
        std::unique_ptr<CDBIterator> pcursor(NewIterator());
        pcursor->Seek(std::make_pair(std::make_pair(DB_TICKETINDEX_SPENT, blockhash), uint256()));
        while (pcursor->Valid()) {
            std::pair<std::pair<char, uint256>, uint256> key;
            if (pcursor->GetKey(key) && key.first.first == DB_TICKETINDEX_SPENT && key.first.second == blockhash) {
                uint256 tickethash = key.second;
                CTicket ticket;
                GetTicket(tickethash, ticket);
                ticketList.push_back(ticket);
                pcursor->Next();
            } else {
                pcursor->Next();
                continue;
            }
        }
        blockIndex = blockIndex->pprev;
    }

    return std::move(ticketList);
}

bool TicketIndex::GetTicket(const uint256& ticketId, CTicket& ticket)
{
    std::pair<std::pair<std::pair<std::pair<uint256, uint256>, uint32_t>, CScript>, CScript> value;
    if (Read(std::make_pair(DB_TICKETINDEX, ticketId), value)) {
        uint256 ticketid = value.first.first.first.first;
        uint256 txid = value.first.first.first.second;
        uint32_t voutindex = value.first.first.second;
        CScript redeemscript = value.first.second;
        CScript scriptpubkey = value.second;
        ticket.setValue(ticketid, txid, voutindex, redeemscript, scriptpubkey);
        return true;
    }
}

bool TicketIndex::GetTicketPrice(CBlockIndex* pindex, uint64_t& ticketPrice)
{
    if (pindex == nullptr) {
        return false;
    }
    ReadTicketPrice(pindex->GetBlockHash(), ticketPrice);
    if (ticketPrice != 0) {
        return true;
    }

    // cannot find ticketPrice, calculate it
    auto prevTicketPrice = ReadPrevTicketPrice(pindex);
    ticketPrice = CalculateTicketPrice(pindex, prevTicketPrice);

    // write into db
    WriteTicketPrice(pindex->GetBlockHash(), ticketPrice);
    return true;
}


uint64_t TicketIndex::CalculateTicketPrice(CBlockIndex* pindex, uint64_t prevPrice)
{
    if (pindex == nullptr || pindex->nHeight < TICKETPRICE_ADJUST_SLOTS) {
        return 25 * COIN;
    }

    if (pindex->nHeight % TICKETPRICE_ADJUST_SLOTS != 0) {
        return prevPrice;
    }

    float ticketPrice;
    auto ticketList = ListTickets(pindex, TICKETPRICE_ADJUST_SLOTS);
    uint32_t sumTickets = ticketList.size();
    uint32_t ticketMargin = TICKETPRICE_ADJUST_SLOTS * 0.1f;

    if (sumTickets >= (TICKETPRICE_ADJUST_SLOTS + ticketMargin)) {
        ticketPrice = prevPrice * 105 / 100;
    } else if (sumTickets <= (TICKETPRICE_ADJUST_SLOTS - ticketMargin)) {
        ticketPrice = prevPrice * 95 / 100;
    } else {
        ticketPrice = prevPrice;
    }

    // minimum price is 1 lava
    return ticketPrice == 0 ? COIN : ticketPrice;
}

bool TicketIndex::WriteTicket(const CTicket ticket, const uint256 blockhash)
{
    CDBBatch batch(*this);
    batch.Write(std::make_pair(std::make_pair(DB_TICKETINDEX_SPENT, blockhash), ticket.GetHash()), 1);
    batch.Write(std::make_pair(DB_TICKETINDEX, ticket.GetHash()), std::make_pair(std::make_pair(std::make_pair(std::make_pair(ticket.GetHash(), ticket.GetTxHash()), ticket.GetIndex()), ticket.GetRedeemScript()), ticket.GetScriptPubkey()));
    return WriteBatch(batch);
}

bool TicketIndex::ReadTicketPrice(const uint256 blockhash, uint64_t& ticketPrice)
{
    std::pair<uint64_t, int> value;
    bool isRead = Read(std::make_pair(DB_TICKETPRICE, blockhash), value);
    ticketPrice = value.first;
    return isRead;
}

bool TicketIndex::WriteTicketPrice(const uint256 blockhash, const uint64_t ticketPrice)
{
    CDBBatch batch(*this);
    batch.Write(std::make_pair(DB_TICKETPRICE, blockhash), ticketPrice);
    return WriteBatch(batch);
}

bool TicketIndex::ReadBestTicket(uint256& blockhash)
{
    std::pair<uint256, int> value;
    bool isRead = Read(DB_TICKETINDEX, value);
    blockhash = value.first;
    return isRead;
}

// record the best ticket into disk for sync.
bool TicketIndex::WriteBestTicket(const uint256 blockhash)
{
    CDBBatch batch(*this);
    batch.Write(DB_TICKETINDEX, blockhash);
    return WriteBatch(batch);
}

uint64_t TicketIndex::ReadPrevTicketPrice(CBlockIndex* pindex)
{
    // initial block
    if (pindex == nullptr || pindex->pprev == nullptr) {
        return 25 * COIN;
    }

    uint64_t prevTicketPrice;
    ReadTicketPrice(pindex->pprev->GetBlockHash(), prevTicketPrice);

    if (prevTicketPrice != 0) {
        return prevTicketPrice;
    }

    // recursive calculate and store ticket price
    prevTicketPrice = CalculateTicketPrice(pindex->pprev, ReadPrevTicketPrice(pindex->pprev));
    WriteTicketPrice(pindex->pprev->GetBlockHash(), prevTicketPrice);

    return prevTicketPrice;
}
