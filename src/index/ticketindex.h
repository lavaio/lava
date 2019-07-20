#ifndef INDEX_TICKETINDEX_H
#define INDEX_TICKETINDEX_H

#include <ticket.h>
#include <pubkey.h>
#include <chainparams.h>
#include <chain.h>
#include <index/base.h>
#include <txdb.h>

class CBlock;
class CBlockIndex;

class TicketIndex final : public BaseIndex
{
protected:
	class DB;

private:
	const std::unique_ptr<DB> m_db;

protected:
	/// Override base class init to migrate from old database.
	bool Init() override;

	bool WriteBlock(const CBlock& block, const CBlockIndex* pindex) override;

	BaseIndex::DB& GetDB() const override;

	const char* GetName() const override { return "ticketindex"; }

public:
	explicit TicketIndex(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

	virtual ~TicketIndex() override;

	bool GetTicket(const uint256& ticketId, CTicket& ticket);

	bool WriteTicket(const CTicket ticket, const uint256 blockhash);

	std::vector<CTicket> ListTickets(const CBlockIndex* pindex, size_t count);
};

/// The global ticket index
extern std::unique_ptr<TicketIndex> g_ticketindex;

#endif // BITCOIN_INDEX_TXINDEX_H