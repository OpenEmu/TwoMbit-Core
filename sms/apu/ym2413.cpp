#include "ym2413.h"
#include "../bus/bus.h"
#include "../cpu/cpu.h"
#include "../system/system.h"
#include "../system/interface.h"

YM2413* YM2413::instance = 0;
void YM2413::Enter() { instance->enter(); }

YM2413::YM2413(_System* sys, Cpu& cpu, Bus& bus)
: sys(sys), cpu(cpu), bus(bus), YM2413CORE()
{
    _address = 0;
    bus.mmio.map(16, this);
    bus.mmio.map(17, this);
    bus.mmio.map(18, this);
}

u8 YM2413::mmio_read(u8 pos) {
    cpu.syncYm();
    if (pos == 18) {
        return 0xF8 | (u8)YM2413CORE::isActivated();
    }
    return cpu.openBus();
}

void YM2413::mmio_write(u8 pos, u8 value) {
    cpu.syncYm();
    if (pos == 16) {
        _address = value & 0xff;
    } else if (pos == 17) {
        YM2413CORE::writeReg(_address, value);
    } else if (pos == 18) {
        YM2413CORE::activate(value & 1);
    }
}

void YM2413::enter() {
    int sample=0;
    while(true) {
        sample = YM2413CORE::YM2413Update();
        sys->interface->audioSample(sample, sample, 1);
        add_cycles(72);
    }
}

void YM2413::add_cycles(u8 cycles) {
    chipClock += cycles * frequency;
    syncCpu();
}

void YM2413::syncCpu() {
    if(getClock() >= 0) {
        co_switch(cpu.getThreadHandle());
    }
}

void YM2413::power() {
    if ( sys->getSystemId() == 1 ) {
        create(Enter, 3);
        instance = this;
    }
    YM2413CORE::reset();
}

