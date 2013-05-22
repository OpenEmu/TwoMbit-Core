
#include "cpu.h"
#include "serializer.h"

void Cpu::serialize(Serializer& s) {

    Core_z80::coreSerialize(s);

    s.integer(vDisplayPos);
    s.integer(oldPause);
    s.integer(vcounter);
    s.integer(hcounterMCL);
    s.integer(_carry);
    s.integer(port_3f);
    s.integer(port_dc);
    s.integer(port_dd);
    s.integer(hcountBuffer);

    s.integer(frameirq.pin_low);
    s.integer(frameirq.enabled);
    s.integer(frameirq._register);

    s.integer(lineirq.pin_low);
    s.integer(lineirq.enabled);
    s.integer(lineirq._register);
    s.integer(lineirq.counter);

    s.integer(nmi.pin_low);
    s.integer(nmi.enabled);
    s.integer(nmi._register);

    s.integer(chipClock);
}
