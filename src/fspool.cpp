#include <fspool.h>
#include <validation.h>

static const char FSPOOL_KEY = 'F';

std::unique_ptr<CFSPool> pfspool;

CFSPool::CFSPool(size_t nCacheSize, bool fMemory, bool fWipe)
    : CDBWrapper(GetDataDir() / "fspool", nCacheSize, fMemory, fWipe) 
{
}

bool CFSPool::ReadFreshFstx(std::vector<CTransaction>& txs, int slotindex){
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
                    txs.emplace_back(CTransaction(transaction));
                }
            }
            pcursor->Next();
        } else {
            break;
        }
    }
    return true;
}

bool CFSPool::WriteFstx(CTransaction tx, int slotindex, uint256 txid){
    auto key = std::make_pair(FSPOOL_KEY, std::make_pair(slotindex, txid));
    if (!Exists(key)) {
        // add tx into cache
        FstxInSlot[slotindex].emplace_back(MakeTransactionRef(tx));
    
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
    std::vector<CTransaction> txs;
    if (!ReadFreshFstx(txs, slotindex)){
        return false;
    }
    
    if (!txs.empty()){
        for (auto fstx:txs){
            FstxInSlot[slotindex].emplace_back(MakeTransactionRef(fstx));
        }
    }

    return true;
}

bool LoadFstx(const uint32_t slotlength)
{
    LogPrintf("%s: Load Fstx from block database...\n", __func__);
    if (chainActive.Tip()==nullptr){
        // new chain
        return true;
    }else{
        // get slot index one by one
        auto slotTip = chainActive.Height() / slotlength + 1;
        for (auto i = 0; i <= slotTip; i++) {
            try {
                if (!pfspool->LoadFstxFromDisk(i))
                    return error("%s: failed to read Fstx from disk, slot: %s", __func__, i);
            } catch (const std::runtime_error& e) {
                return error("%s: failure: %s", __func__, e.what());
            }
        }
        return true;
    }
}
