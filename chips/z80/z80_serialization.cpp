
#include "z80.h"
#include "serializer.h"

void Core_z80::coreSerialize(Serializer& s) {
    s.integer(opcode);
    s.integer(fetch_mode);
    s.integer(stop);
    s.integer(nmiPending);
    s.integer(irqPending);
    s.integer(intr.imode);
    s.integer(intr.delay);
    s.integer(intr.iff1);
    s.integer(intr.iff2);

    s.integer(reg.w.AF);
    s.integer(reg.w.BC);
    s.integer(reg.w.DE);
    s.integer(reg.w.HL);
    s.integer(reg.w.IX);
    s.integer(reg.w.IY);
    s.integer(reg.w.SP);
    s.integer(reg.w.PC);

    s.integer(reg_al.w.AF);
    s.integer(reg_al.w.BC);
    s.integer(reg_al.w.DE);
    s.integer(reg_al.w.HL);
    s.integer(reg_al.w.IX);
    s.integer(reg_al.w.IY);
    s.integer(reg_al.w.SP);
    s.integer(reg_al.w.PC);

    s.integer(reg_r);
    s.integer(reg_i);
    s.integer(data8);
    s.integer(displacement);
    s.integer(data16);
    s.integer(mdr);
}
