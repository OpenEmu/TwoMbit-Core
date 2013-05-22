
#include "bus.h"
#include "eeprom_93c46.h"
#include "serializer.h"

void Bus::serialize(Serializer& s) {
    s.integer(port_3e);

    for(unsigned i = 0; i < 2; i++) {
        s.integer(slot[i].ramSelector);
        s.integer(slot[i].page0bank);
        s.integer(slot[i].page1bank);
        s.integer(slot[i].page2bank);
        s.integer(slot[i].page3bank);
        s.integer(slot[i].page4bank);
        s.integer(slot[i].page0flip);
        s.integer(slot[i].page1flip);
    }
    eeprom->serialize(s);
}
