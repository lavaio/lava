#include "slotinfo.h"
#include <validation.h>

SlotInfo SlotInfo::currentSlotInfo()
{
    LOCK(cs_main);

    int index = pticketview->SlotIndex();
    auto price = pticketview->TicketPriceInSlot(index);
    auto count = (uint64_t)pticketview->GetTicketsBySlotIndex(index).size();
    auto lockTime = pticketview->LockTime(index);

    return SlotInfo {
        index, lockTime, price, count
    };
}
