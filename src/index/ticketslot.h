#ifndef INDEX_TICKETSLOT_H
#define INDEX_TICKETSLOT_H

#include <index/ticketindex.h>
#include <pubkey.h>
#include <chainparams.h>
#include <chain.h>
#include <index/base.h>
#include <txdb.h>

class CBlock;
class CBlockIndex;

class TicketSlot final : public BaseIndex
{
protected:
    class DB;

private:
    const std::unique_ptr<DB> m_db;

protected:
    // Override base class init to migrate from old database.
    bool Init() override;

    bool WriteBlock(const CBlock& block, const CBlockIndex* pindex) override;

    BaseIndex::DB& GetDB() const override;

    const char* GetName() const override { return "ticketslot"; }

public:
    explicit TicketSlot(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    virtual ~TicketSlot() override;

    uint64_t GetTicketPrice(const CBlockIndex* pindex);
    
    bool ReadTicketPrice(const uint256 blockhash, uint64_t& price);
    
    bool WriteTicketPrice(const uint256 blockhash, const uint64_t rate);
};

// The global ticket slot
extern std::unique_ptr<TicketSlot> g_ticket_slot;

#endif // !INDEX_TICKETSLOT_H
