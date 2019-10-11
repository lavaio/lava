#ifndef SLOTINFO_H
#define SLOTINFO_H

#include <amount.h>

struct SlotInfo
{
    int index;
    int lockTime;
    CAmount price;
    uint64_t count;

    static SlotInfo currentSlotInfo();
};

#endif // SLOTINFO_H
