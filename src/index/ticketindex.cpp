#include <index/ticketindex.h>
#include <primitives/block.h>
#include <chain.h>
#include <validation.h>

std::unique_ptr<TicketIndex> g_ticket;
constexpr char DB_TICKETINDEX_SPENT = 'B';
constexpr char DB_TICKETINDEX = 'T';

TicketIndex::TicketIndex(size_t n_cache_size, bool f_memory, bool f_wipe)
    :CDBWrapper(GetDataDir() / "indexes" / "ticketindex", n_cache_size, f_memory, f_wipe)
{
}

TicketIndex::~TicketIndex()
{
}

bool TicketIndex::ConnectBlock(const CBlock& block, CBlockIndex* pindex)
{
    //key:B + blockhash + tickethash
    //value: 1

    //key: T + tickethash
    //value: cticket
	uint256 blockhash;
	ReadBestTicket(blockhash);

	// pindex->pprev is the tip of the ActiveChain
	// it means that pindex is 1 block higher than ActiveChain.
	if (pindex->GetBlockHash() != blockhash){
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
				bool isWrited = WriteTicket(*ticket,thisblockhash);
				if (!isWrited)
					return false;
			}
		}
	}
	WriteBestTicket(pindex->GetBlockHash());
    return true;
}

// erase the undo_ticket in level DB
// in fact, Erase is allowed to be failed, as tickets are searched by blockhash.
bool TicketIndex::DisconnectBlock(const CBlock& block, CBlockIndex* pindexDelete)
{
	uint256 blockhash;
	ReadBestTicket(blockhash);

	if(pindexDelete->GetBlockHash()!=blockhash){
		for (int i=0; i<block.vtx.size(); i++){
			CTransaction tx = *block.vtx[i];
			if (tx.IsTicketTx()){
				CTicketRef ticket = tx.Ticket();
				bool isErasedB = Erase(std::make_pair(std::make_pair(DB_TICKETINDEX_SPENT, blockhash),ticket->GetHash()), false);
				bool isErasedT = Erase(std::make_pair(DB_TICKETINDEX,ticket->GetHash()), false);
				if (!isErasedB || !isErasedT){
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

bool TicketIndex::GetTicket(const uint256& ticketId, CTicket& ticket)
{
	return Read(std::make_pair(DB_TICKETINDEX, ticketId), ticket);
}

bool TicketIndex::WriteTicket(const CTicket ticket, const uint256 blockhash)
{
	CDBBatch batch(*this);
	batch.Write(std::make_pair(std::make_pair(DB_TICKETINDEX_SPENT, blockhash),ticket.GetHash()), 1);
	batch.Write(std::make_pair(DB_TICKETINDEX,ticket.GetHash()), ticket);
	return WriteBatch(batch);
}

bool TicketIndex::ReadBestTicket(uint256& blockhash){
	std::pair<uint256, int> value;
	bool isRead = Read(DB_TICKETINDEX, value);
	blockhash = value.first;
	return isRead;
}

// record the best ticket into disk for sync.
bool TicketIndex::WriteBestTicket(uint256 blockhash){
	CDBBatch batch(*this);
	batch.Write(DB_TICKETINDEX, blockhash);
	return WriteBatch(batch);
}