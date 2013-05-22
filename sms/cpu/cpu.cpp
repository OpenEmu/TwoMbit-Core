
#include "cpu.h"
#include "../system/system.h"
#include "../vdp/vdp.h"
#include "../bus/bus.h"
#include "../apu/sn76489.h"
#include "../apu/ym2413.h"
#include "../system/interface.h"

Cpu* Cpu::instance1 = 0;
void Cpu::Enter1() { instance1->enter(); }
Cpu* Cpu::instance2 = 0;
void Cpu::Enter2() { instance2->enter(); }

Cpu::Cpu(_System* sys, Bus& bus, Vdp& vdp, SN76489& sn76489, YM2413& ym2413, _Input& input, Gear2Gear& gear2gear)
:
sys(sys),
bus(bus),
vdp(vdp),
sn76489(sn76489),
ym2413(ym2413),
input(input),
Core_z80(),
frameirq(this, &vdp, sys),
lineirq(this, &vdp, sys),
nmi(this, &vdp, sys, &input),
gear2gear(gear2gear)
{
	bus.mmio.map(1, this);
	bus.mmio.map(2, this);
	bus.mmio.map(3, this);
	bus.mmio.map(6, this);
	bus.mmio.map(7, this);
    bus.mmio.map(8, this);
    bus.mmio.map(15, this);

	clockVdp = vdp.getClockPointer();
    clockSn = sn76489.getClockPointer();
    clockYm = ym2413.getClockPointer();
	clockG2G = gear2gear.getClockPointer();
}

void Cpu::syncVdp() {
    if(*clockVdp < 0) {
        co_switch(vdp.getThreadHandle());
    }
}

void Cpu::syncSn() {
    if(*clockSn < 0) {
        co_switch(sn76489.getThreadHandle());
    }
}

void Cpu::syncYm() {
    if(*clockYm < 0) {
        co_switch(ym2413.getThreadHandle());
    }
}

void Cpu::syncGear2Gear() {
    if (!sys->emulateG2G()) return;
    if(*clockG2G < 0) {
        co_switch(gear2gear.getThreadHandle());
    }
}

void Cpu::addclocks(u8 cycles) {
    *clockVdp -= cycles * frequency;
    *clockSn -= cycles * frequency;
    if (sys->emulateG2G()) {
        *clockG2G -= cycles * frequency;
    } else if (sys->isYamahaSupported()) {
        *clockYm -= cycles * frequency;
    }
}

u8 Cpu::openBus() {
    if (sys->getUnit() == _System::SG1000) {
        return 0xff;
    } if (sys->getRevision() == _System::SMS1) {
		return Core_z80::openBus();
	} if (sys->getRevision() == _System::SMS2) {
		return 0xff;
	}
	return Core_z80::openBus();
}

u8 Cpu::memRead(u16 addr) {
	u8 data = bus.read_from(addr & 0xFFFF);
	return data;
}

void Cpu::memWrite(u16 addr, u8 value) {
	bus.write_to(addr & 0xFFFF, value);
}

u8 Cpu::ioRead(u8 addr) {
	return bus.mmio.read(addr);
}

void Cpu::ioWrite(u8 addr, u8 value) {
	bus.mmio.write(addr, value);
}

u8 Cpu::mmio_read(u8 pos) {
	switch(pos) {
		case 1: return read_port_3F();
		case 2: return read_port_7E();
		case 3: return read_port_7F();
		case 6: return read_port_DC();
		case 7: return read_port_DD();
		case 8: return read_port_GG_0x0();
        case 15: return openBus();
	}
}

void Cpu::mmio_write(u8 pos, u8 value) {
	switch(pos) {
		case 1: write_port_3F(value); break;
		case 2: write_port_7E(value); break;
		case 3: write_port_7F(value); break;
		case 6: write_port_DC(value); break;
		case 7: write_port_DD(value); break;
		case 8: write_port_GG_0x0(value); break;
		case 15: break;
	}
}

u8 Cpu::read_port_GG_0x0() {
    u8 out = 0;
    out = doSetRes( (input.readPort( sys->getSystemId() == 1 ? _Input::Port1 : _Input::Port2, _Input::Start) & 1), 7, out);
    out = doSetRes( sys->_region() != _System::JAPAN, 6, out);
    out = doSetRes( sys->_region() == _System::EUROPE, 5, out);
    return out;
}
void Cpu::write_port_GG_0x0(u8 value) {
    //read only
}

u8 Cpu::read_port_3F() {
	return openBus();
}

u8 Cpu::read_port_DC() {
    port_dc = doSetRes( ( input.readPort(sys->getSystemId() == 1 ? _Input::Port1 : _Input::Port2, _Input::Up) & 1 ), 0, port_dc);
	port_dc = doSetRes( ( input.readPort(sys->getSystemId() == 1 ? _Input::Port1 : _Input::Port2, _Input::Down) & 1 ), 1, port_dc);
	port_dc = doSetRes( ( input.readPort(sys->getSystemId() == 1 ? _Input::Port1 : _Input::Port2, _Input::Left) & 1 ), 2, port_dc);
	port_dc = doSetRes( ( input.readPort(sys->getSystemId() == 1 ? _Input::Port1 : _Input::Port2, _Input::Right) & 1 ), 3, port_dc);
    port_dc = doSetRes( ( input.readPort(sys->getSystemId() == 1 ? _Input::Port1 : _Input::Port2, _Input::A) & 1 ), 4, port_dc);
    if ( (sys->getUnit() == _System::SG1000) || ( (port_3f & 1) != 0) ) {
        port_dc = doSetRes( ( input.readPort(sys->getSystemId() == 1 ? _Input::Port1 : _Input::Port2, _Input::B) & 1 ), 5, port_dc);
    }

	if (sys->getUnit() == _System::GG) {
        port_dc = doSetRes(gear2gear.getExt(0), 6, port_dc);
        port_dc = doSetRes(gear2gear.getExt(1), 7, port_dc);
	} else {
        port_dc = doSetRes( ( input.readPort(_Input::Port2, _Input::Up) & 1 ), 6, port_dc);
        port_dc = doSetRes( ( input.readPort(_Input::Port2, _Input::Down) & 1 ), 7, port_dc);
    }
	return port_dc;
}

u8 Cpu::read_port_DD() {
    if (sys->getUnit() == _System::GG) {
        port_dd = doSetRes(gear2gear.getExt(2), 0, port_dd);
        port_dd = doSetRes(gear2gear.getExt(3), 1, port_dd);
        port_dd = doSetRes(gear2gear.getExt(4), 2, port_dd);
        if ( (port_3f & 4) != 0) {
            port_dd = doSetRes(gear2gear.getExt(5), 3, port_dd);
        }
        port_dd = doSetRes(gear2gear.getExt(6), 7, port_dd);
        return port_dd;
	}
	port_dd = doSetRes( ( input.readPort(_Input::Port2, _Input::Left) & 1 ), 0, port_dd);
	port_dd = doSetRes( ( input.readPort(_Input::Port2, _Input::Right) & 1 ), 1, port_dd);
	port_dd = doSetRes( ( input.readPort(_Input::Port2, _Input::A) & 1 ), 2, port_dd);
	if ((sys->getUnit() == _System::SG1000) || ( (port_3f & 4) != 0)) {
		port_dd = doSetRes( ( input.readPort(_Input::Port2, _Input::B) & 1 ), 3, port_dd);
	}
	//reset pin is written via reset()
	//TH is written via setTH
	return port_dd;
}

void Cpu::setTH(_Input::Port port, bool state) {

    if ( port == _Input::Port1 && ( (port_3f & 2) != 0 ) ) {
        if(state == 0) latchHcount();
        port_dd = doSetRes( state, 6, port_dd);

    } else if ( port == _Input::Port2 && ( (port_3f & 8) != 0 ) ) {
        if(state == 0) latchHcount();
		port_dd = doSetRes( state, 7, port_dd);
	}
}

void Cpu::write_port_3F(u8 value) {
	if ( ( (value & 2) > (port_3f & 2) )
	|| ( (value & 8) > (port_3f & 8) ) ) {
		latchHcount();
	}

	bool port1th = (value & 0x20) != 0;
    port1th = sys->_region() == _System::JAPAN ? !port1th : port1th;
	bool port2th = (value & 0x80) != 0;
    port2th = sys->_region() == _System::JAPAN ? !port2th : port2th;
	bool port1tr = (value & 0x10) != 0;
	bool port2tr = (value & 0x40) != 0;

    if( (value & 8) == 0) port_dd = doSetRes(port2th, 7, port_dd);
	if( (value & 4) == 0) port_dd = doSetRes(port2tr, 3, port_dd);
    if( (value & 2) == 0) port_dd = doSetRes(port1th, 6, port_dd);
	if( (value & 1) == 0) port_dc = doSetRes(port1tr, 5, port_dc);

    input.updateSportspad(port_3f, value);
	port_3f = value;
}

void Cpu::write_port_DC(u8 value) {
	//no writes c0 - ff
}

void Cpu::write_port_DD(u8 value) {
	//no writes c0 - ff
}

u8 Cpu::read_port_7E() {
	return vdp.getInternalVCounter( vcounter );
}

void Cpu::latchHcount() {
	// F5h ---- FFh | 00h --------- 20h --LCD-- 6FH -------- 93H | E9H ---- F4H
    u16 _hmcl =  hcounterMCL + _carry;

    hcountBuffer = (_hmcl + (_hmcl > 0 ? -1 : 683) ) >> 2;
	if (hcountBuffer > 0x93) {
		hcountBuffer += 0x55;
	}
}

u8 Cpu::read_port_7F() {
	return hcountBuffer;
}

void Cpu::write_port_7E(u8 value) {
    syncSn();
	sn76489.write( value );
}

void Cpu::write_port_7F(u8 value) {
    syncSn();
	sn76489.write( value );
}

void Cpu::enter() {

	while(true) {
        if(sys->getSync() == _System::SYNC_CPU_EDGE) {
            sys->setSync(_System::SYNC_ALL);
            sys->exit();
        }
		nextOpcode();
	}
}

void Cpu::power() {
    if (sys->getSystemId() == 1 ) {
        create(Enter1, 3);
        instance1 = this;
    } else if (sys->getSystemId() == 2 ) {
        create(Enter2, 3);
        instance2 = this;
    }
	Core_z80::power();
	intr.imode = 1;
	vcounter = hcounterMCL = 0;
	port_dc = port_dd = 0xff;
	port_3f = 0xff;
	frameirq.init();
	lineirq.init();
    nmi.init();
	hcountBuffer = 0;
    vDisplayPos = 0;
	oldPause = 1;
    _carry = false;
}

void Cpu::reset() {
	if ((sys->getUnit() == _System::SG1000) || (sys->getRevision() == _System::SMS1)) {
        port_dd = doSetRes(false, 4, port_dd);
	}
}

inline bool Cpu::checkIrq() {
    syncVdp();
    return frameirq.isRegister() || lineirq.isRegister();
}

bool Cpu::checkNmi() {
    if (!sys->isGGMode()) {
        if (nmi.isRegister()) {
            nmi.resetRegister();
            return true;
        }
    } else if (sys->emulateG2G()) {
        syncGear2Gear();
        return gear2gear.isNmi();
    }
	return false;
}

void Cpu::updateDisplayVPos() {
    Vdp::VMode vmode = vdp.getVMode( vcounter );

    switch( vmode ) {
        case Vdp::DISPLAY:
        case Vdp::BORDER_BOTTOM:
        case Vdp::BORDER_TOP:
            vDisplayPos++;
            break;
        case Vdp::BLANKING:
            vDisplayPos = 0;
            break;
    }
}

void Cpu::sync(u8 cycles) {
    if (cycles == 0) return;

    addclocks(cycles);
    unsigned MCL = cycles * 3;

    MCL += _carry;
    _carry = MCL & 1;
    MCL >>= 1;

    while (MCL--) {
        if ( input.isCustomInput() ) {
            updateCustomInput();
        }
        hcounterMCL+=2;

		if (hcounterMCL == 636) {
            if (sys->emulateG2G()) {
                syncGear2Gear();
                sys->G2Gswitch();
            }
            syncVdp();
            updateDisplayVPos();
            if (++vcounter >= (sys->_isNTSC() ? 262 : 313)  ) { //frame complete
				port_dd = doSetRes(true, 4, port_dd); //release reset button, for SMS2 it is released already
                if (sys->getSystemId() == 1 ) {
                    sys->exit();
                }
				vcounter = 0;
			}
            if (sys->isGGMode()) lineirq.poll();
            if (vcounter == (sys->_isNTSC() ? 261 : 312)  ) nmi.poll();
		}
        else if (hcounterMCL == 684) {
            syncSn();
            syncYm();
			hcounterMCL = 0;
		}
        else if (hcounterMCL == 634) frameirq.poll();
        else if (sys->isSmsMode() && hcounterMCL == 638) lineirq.poll();
	}
}

void Cpu::updateCustomInput() {
    if ( (hcounterMCL % 2) == 0) {
        input.tickGuns();
        input.tickJapSportspad(_Input::Port1);
        input.tickJapSportspad(_Input::Port2);
    }

    input.updatePaddle();
}

void Cpu::Nmi::poll() {
    if (sys->isGGMode()) return;
    bool pause = !(sys->interface->getInputState(_Input::PortNone, _Input::IdMisc, _Input::Pause) & 1);
    if (cpu->oldPause > pause) _register = true;
    cpu->oldPause = pause;
}

void Cpu::FrameIrq::poll() {
	u8 height = vdp->getScreenHeight();

    if (	(height == 192 && cpu->vcounter == 192)
        ||	(height == 224 && cpu->vcounter == 224)
        ||	(height == 240 && cpu->vcounter == 240)) {

        pin_low = true;
        cpu->syncVdp();
        vdp->setIrqState();
        if (enabled ) {
            _register = true;
        }
	}
}

void Cpu::LineIrq::poll() {
	u8 height = vdp->getScreenHeight();

    if (	(height == 192 && cpu->vcounter <= 192)
        ||	(height == 224 && cpu->vcounter <= 224)
        ||	(height == 240 && cpu->vcounter <= 240)) {

        if ( --counter == 0xFF ) {
            cpu->syncVdp();
            counter = vdp->getLineCounter();
            pin_low = true;

            if (enabled) {
                _register = true;
            }
        }
    } else {
        cpu->syncVdp();
        counter = vdp->getLineCounter();
	}
}
