#include <index/ticketindex.h>
#include <primitives/block.h>
#include <chain.h>
#include <validation.h>

std::unique_ptr<TicketIndex> g_ticketindex;
constexpr char DB_TICKETINDEX_SPENT = 'B';
constexpr char DB_TICKETINDEX = 'T';

class TicketIndex::DB : public BaseIndex::DB
{
public:
	explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

	bool GetTicket(const uint256& ticketId, CTicket& ticket);

	bool WriteTicket(const CTicket ticket, const uint256 blockhash);

	std::vector<CTicket> ListTickets(CBlockIndex* pindex, size_t count);
};

TicketIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
	BaseIndex::DB(GetDataDir() / "indexes" / "ticketindex", n_cache_size, f_memory, f_wipe)
{}

bool TicketIndex::DB::GetTicket(const uint256& ticketId, CTicket& ticket)
{
	std::pair<std::pair<std::pair<std::pair<uint256, uint256>, uint32_t>, CScript>, CScript> value;
	if(Read(std::make_pair(DB_TICKETINDEX, ticketId), value)){
		uint256 ticketid = value.first.first.first.first;
		uint256 txid = value.first.first.first.second;
		uint32_t voutindex = value.first.first.second;
		CScript redeemscript = value.first.second;
		CScript scriptpubkey = value.second;
		ticket.setValue(ticketid, txid, voutindex, redeemscript, scriptpubkey);
		return true;
	}
}

bool TicketIndex::DB::WriteTicket(const CTicket ticket, const uint256 blockhash)
{
	CDBBatch batch(*this);
	batch.Write(std::make_pair(std::make_pair(DB_TICKETINDEX_SPENT, blockhash),ticket.GetHash()), 1);
	batch.Write(std::make_pair(DB_TICKETINDEX,ticket.GetHash()),std::make_pair(std::make_pair(std::make_pair(std::make_pair(ticket.GetHash(),ticket.GetTxHash()),ticket.GetIndex()),ticket.GetRedeemScript()),ticket.GetScriptPubkey()));
	return WriteBatch(batch);
}

BaseIndex::DB& TicketIndex::GetDB() const { return *m_db; }

// this API is used to calculate ticket price.
std::vector<CTicket> TicketIndex::DB::ListTickets(CBlockIndex* pindex, size_t count)
{
	std::vector<CTicket> ticketList;
	CBlockIndex* blockIndex = pindex;

	for (size_t index=0; index<count; index++){
		if (blockIndex==nullptr)
			break;
		uint256 blockhash = *blockIndex->phashBlock;
		std::unique_ptr<CDBIterator> pcursor(NewIterator());
		pcursor->Seek(std::make_pair(std::make_pair(DB_TICKETINDEX_SPENT, blockhash),uint256()));
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

TicketIndex::TicketIndex(size_t n_cache_size, bool f_memory, bool f_wipe)
	: m_db(MakeUnique<TicketIndex::DB>(n_cache_size, f_memory, f_wipe))
{}

TicketIndex::~TicketIndex()
{
}

bool TicketIndex::Init()
{
	return BaseIndex::Init();
}

bool TicketIndex::WriteBlock(const CBlock& block, const CBlockIndex* pindex)
{
	//key:B + blockhash + tickethash
	//value: 1

	//key: T + tickethash
	//value: cticket
	auto thisblockhash = block.GetHash();
	for (unsigned int i = 0; i < block.vtx.size(); i++){
		const CTransaction& tx = *(block.vtx[i]);
		if (tx.IsTicketTx()){
			CTicketRef ticket;
			try{
				ticket = tx.Ticket();
			}catch(...){
				continue;
			}			
			// write the ticket into DB
			bool isWrited = m_db->WriteTicket(*ticket,thisblockhash);
			if (!isWrited)
				return false;
		}
	}
	return true;
}

bool TicketIndex::GetTicket(const uint256& ticketId, CTicket& ticket){
	return m_db->GetTicket(ticketId, ticket);
}

bool TicketIndex::WriteTicket(const CTicket ticket, const uint256 blockhash){
	return m_db->WriteTicket(ticket, blockhash);
}

std::vector<CTicket> TicketIndex::ListTickets(CBlockIndex* pindex, size_t count){
	return m_db->ListTickets(pindex, count);
}