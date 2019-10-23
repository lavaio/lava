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
    bool ReadFreshFs(CMutableTransaction& tx, int slotindex);

    /** 
    * write a firestone used transaction into the fspool.
    * @param[in]   tx       the transaction, in which a new fresh firestone is used.
    * @param[in]   slotindex       the firestone is USABLE at this slot.
    * @return      true if written.
    */
    bool WriteFs(CMutableTransaction tx, int slotindex, uint256 txid);
    
    /** 
    * remove the firestone used txs in the fspool, at a slotindex.
    * @param[in]   slotindex      remove those transaction in fspool at this slotindex.
    * @return      true if removed.
    */
    bool RemoveSlot(int slotindex);
};

#endif