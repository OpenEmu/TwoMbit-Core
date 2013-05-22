
#include "system.h"
#include "serializer.h"

void _System::serialize(Serializer& s) {
    s.array(vram.handle(), vram.size());
    s.array(wram.handle(), wram.size());
    s.array(wramSG.handle(), wramSG.size());

    if (sram.size() != 0) {
        s.array(sram.handle(), sram.size());
    }
    if (cartWram.size() != 0) {
        s.array(cartWram.handle(), cartWram.size());
    }
//    unsigned _unit = unit;
//    s.integer(_unit);
//    unit = (Unit)_unit;

    unsigned _rev = revision;
    s.integer(_rev);
    revision = (Revision)_rev;

    unsigned _reg = region;
    s.integer(_reg);
    region = (Region)_reg;

    s.integer(yamahaSupported);
}
