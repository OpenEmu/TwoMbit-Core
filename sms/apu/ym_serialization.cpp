
#include "ym2413.h"
#include "serializer.h"

void YM2413::serialize(Serializer& s) {
    YM2413CORE::coreSerialize(s);
    s.integer(_address);
    s.integer(chipClock);
}
