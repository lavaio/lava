#include <fspool.h>
#include <validation.h>

static const char FSPOOL_KEY = 'F';

CFSPool::CFSPool(size_t nCacheSize, bool fMemory, bool fWipe)
    : CDBWrapper(GetDataDir() / "fspool", nCacheSize, fMemory, fWipe) 
{
}

bool CFSPool::ReadFreshFs(CMutableTransaction& tx, int slotindex){
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
                    if (!pcoinsTip->AccessCoin(transaction.vin[0].prevout).IsSpent()){
                        tx = transaction;
                        return true;
                    }
                }
            }
            pcursor->Next();
        } else {
            break;
        }
    }
    return false;
}

bool CFSPool::WriteFs(CMutableTransaction tx, int slotindex, uint256 txid){
    return Write(std::make_pair(FSPOOL_KEY, std::make_pair(slotindex, txid)), tx);
}

bool CFSPool::RemoveSlot(int slotindex){
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