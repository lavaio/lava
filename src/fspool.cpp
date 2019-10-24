#include <fspool.h>
#include <validation.h>

static const char FSPOOL_KEY = 'F';

CFSPool::CFSPool(size_t nCacheSize, bool fMemory, bool fWipe)
    : CDBWrapper(GetDataDir() / "fspool", nCacheSize, fMemory, fWipe) 
{
}

bool CFSPool::ReadFreshFstx(std::vector<CMutableTransaction>& txs, int slotindex){
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(FSPOOL_KEY,std::make_pair(slotindex, uint256())));

    // Read fresh fstx
    while (pcursor->Valid()) {
        std::pair<char, std::pair<int, uint256>> key;
        if (pcursor->GetKey(key) && key.first == FSPOOL_KEY) {
            if (key.second.first == slotindex){
                CMutableTransaction transaction;
                if (pcursor->GetValue(transaction)) {
                    // check the firestone is not used.
                    //if (!pcoinsTip->AccessCoin(transaction.vin[0].prevout).IsSpent()){}
                    txs.emplace_back(transaction);
                }
            }
            pcursor->Next();
        } else {
            break;
        }
    }
    return true;
}

bool CFSPool::WriteFstx(CMutableTransaction tx, int slotindex, uint256 txid){
    auto key = std::make_pair(FSPOOL_KEY, std::make_pair(slotindex, txid));
    if (!Exists(key)) {
        // add tx into cache
        CTransaction Tx(tx);
        FstxInSlot[slotindex].emplace_back(MakeTransactionRef(Tx));
    
        // add tx into disk
        return Write(std::make_pair(FSPOOL_KEY, std::make_pair(slotindex, txid)), tx);
    }
    return true;
}

bool CFSPool::RemoveSlot(int slotindex){
    // clear the fstx in cache
    FstxInSlot[slotindex].clear();

    // clear the disk
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(FSPOOL_KEY,std::make_pair(slotindex, uint256())));

    while (pcursor->Valid()) {
        std::pair<char, std::pair<int, uint256>> key;
        if (pcursor->GetKey(key) && key.first == FSPOOL_KEY) {
            if (key.second.first == slotindex){
                Erase(key);
            }
            pcursor->Next();
        } else {
            break;
        }
    }
    return true;
}

std::vector<CTransactionRef> CFSPool::GetFstxBySlotIndex(const int slotIndex){
    return FstxInSlot[slotIndex];
}

bool CFSPool::LoadFstxFromDisk(const int slotindex){
    // txs is the fstx set in slotindex
    std::vector<CMutableTransaction> txs;
    if (!ReadFreshFstx(txs, slotindex)){
        return false;
    }
    
    if (!txs.empty()){
        for (auto fstx:txs){
            CTransaction Tx(fstx);
            FstxInSlot[slotindex].emplace_back(MakeTransactionRef(Tx));
        }
    }

    return true;
}