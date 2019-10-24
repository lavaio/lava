#ifndef LAVA_FSPOOL_H
#define LAVA_FSPOOL_H

#include <dbwrapper.h>
#include <primitives/transaction.h>
//#include <validation.h>

/** 
* Abstract view on the firestone used transaction dataset. 
*/
class CFSPool : public CDBWrapper
{
public:
     CFSPool(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    ~CFSPool() = default;

    /** 
    * Read a transaction, in which a new fresh firestone is used, from the fspool.
    * @param[out]  tx       the transaction, in which a new fresh firestone is used.
    * @param[in]   slotindex       read the tx using the firestone, and the firestone is USABLE at this slot.
    * @return      true if read.
    */
    bool ReadFreshFstx(std::vector<CMutableTransaction>& txs, int slotindex);

    /** 
    * write a firestone used transaction into the fspool.
    * @param[in]   tx       the transaction, in which a new fresh firestone is used.
    * @param[in]   slotindex       the firestone is USABLE at this slot.
    * @return      true if written.
    */
    bool WriteFstx(CMutableTransaction tx, int slotindex, uint256 txid);
    
    /** 
    * remove the firestone used txs in the fspool, at a slotindex.
    * @param[in]   slotindex      remove those transaction in fspool at this slotindex.
    * @return      true if removed.
    */
    bool RemoveSlot(int slotindex);

    std::vector<CTransactionRef> GetFstxBySlotIndex(const int slotIndex);

    /** 
    * Load the fstx set at slotindex via fspool read.
    * @param[in]   slotindex, from which fstx is load.
    */
    bool LoadFstxFromDisk(const int slotindex);

private:
    /** This map records fstx in each slot, one slot is 2048 blocks.*/
    std::map<int, std::vector<CTransactionRef>> FstxInSlot;  
};

#endif