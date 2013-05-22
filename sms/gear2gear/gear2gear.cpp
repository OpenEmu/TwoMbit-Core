
#include "gear2gear.h"
#include "../system/system.h"
#include "../cpu/cpu.h"
#include "../bus/bus.h"

Gear2Gear* Gear2Gear::instance1 = 0;
void Gear2Gear::Enter1() { instance1->enter(); }
Gear2Gear* Gear2Gear::instance2 = 0;
void Gear2Gear::Enter2() { instance2->enter(); }
i64 Gear2Gear::sysClock = 0;

Gear2Gear::Gear2Gear(_System* sys, Cpu& cpu, Bus& bus)
:
sys(sys),
cpu(cpu),
bus(bus)
{
	bus.mmio.map(9, this);
	bus.mmio.map(10, this);
	bus.mmio.map(11, this);
	bus.mmio.map(12, this);
	bus.mmio.map(13, this);
}

u8 Gear2Gear::mmio_read(u8 pos) {
    cpu.syncGear2Gear();
    sys->G2Gswitch();
	switch(pos) {
		case 9: return read_port_GG_0x1();
		case 10: return read_port_GG_0x2();
		case 11: return read_port_GG_0x3();
		case 12: return read_port_GG_0x4();
		case 13: return read_port_GG_0x5();
	}
}

void Gear2Gear::mmio_write(u8 pos, u8 value) {
    cpu.syncGear2Gear();
    sys->G2Gswitch();
	switch(pos) {
		case 9: write_port_GG_0x1(value); break;
		case 10: write_port_GG_0x2(value); break;
		case 11: write_port_GG_0x3(value); break;
		case 12: write_port_GG_0x4(value); break;
		case 13: write_port_GG_0x5(value); break;
	}
}

void Gear2Gear::power() {
    if (sys->getSystemId() == 1 ) {
        create(Enter1, 3);
        instance1 = this;
    } else if (sys->getSystemId() == 2 ) {
        create(Enter2, 3);
        instance2 = this;
    }
    transfer.delayS = 0;
    transfer.delayP = 0;
    transfer.speed = B4800;
    transfer.sendCount = 0;
    transfer.receiveCount = 0;
    transfer.raiseNmi = false;
    transfer.parallelNmi = false;
    transfer.serialNmi = true;
    transfer.bufferFill = false;
    transfer.sendBuffer = 0;
    transfer.receiveBuffer = 0;

    sysClock = 59736 * 60.5; //let the second GG begin 60 frames later

    ggMmio[1].data = 0x7f;
    ggMmio[2].data = 0xff;
    ggMmio[3].data = 0x00;
    ggMmio[4].data = 0xff;
    ggMmio[5].data = 0x00;

	if (!sys->emulateG2G()) {
     //   _setBit(ggMmio[5].data, 2);
	}
}

void Gear2Gear::enter() {
    _System* friendSystem = sys->getFriendSystem();
    while(true) {
        if(transfer.delayS) {
            if (--transfer.delayS == 0) {
                sys->G2Gswitch();
                friendSystem->g2gTransfer(4, _getBit(transfer.sendBuffer, 7), SERIAL );
                if (transfer.sendCount == 1) {
                    _resBit(ggMmio[5].data, 0);
                }

                if (--transfer.sendCount > 0) {
                    transfer.sendBuffer <<= 1;
                    transfer.delayS = transfer.speed;
                } else {
                    if (transfer.bufferFill) {
                        transfer.bufferFill = false;
                        transfer.sendCount = 8;
                        transfer.delayS = transfer.speed;
                        transfer.sendBuffer = ggMmio[3].data;
                        _setBit(ggMmio[5].data, 0);
                    }
                }
            }
        }

        if(transfer.delayP) {
            if (--transfer.delayP == 0) {
                sys->G2Gswitch();
                for (u8 bit=0; bit<=6; bit++) {
                    if (isOutputBit(bit)) {
                        friendSystem->g2gTransfer(bit, _getBit(ggMmio[1].data, bit), PARALLEL );
                    }
                }
            }
        }
        addClocks(1);
    }
}

void Gear2Gear::addClocks(u8 cycles) {
    chipClock += cycles * frequency;
    if (sys->getSystemId() == 1 ) {
        sysClock -= cycles;
    } else {
        sysClock += cycles;
    }
    syncCpu();
}

void Gear2Gear::syncCpu() {
    if(getClock() >= 0) {
        co_switch(cpu.getThreadHandle());
    }
}

u8 Gear2Gear::read_port_GG_0x1() {
    return ggMmio[1].data;
}

u8 Gear2Gear::read_port_GG_0x2() {
    return ggMmio[2].data;
}

u8 Gear2Gear::read_port_GG_0x3() {
    return ggMmio[3].data;
}

u8 Gear2Gear::read_port_GG_0x4() {
    transfer.serialNmi = true;
    _resBit(ggMmio[5].data, 1);
    return ggMmio[4].data;
}

u8 Gear2Gear::read_port_GG_0x5() {
    return ggMmio[5].data;
}

u8 Gear2Gear::sentToReceive(u8 bit) {
    if (bit == 0) return 2;
    if (bit == 1) return 3;
    if (bit == 2) return 0;
    if (bit == 3) return 1;
    if (bit == 4) return 5;
    if (bit == 5) return 4;
    return bit;
}

void Gear2Gear::_receive(u8 bit, bool value, Mode _mode) {
    bit = sentToReceive(bit);

    if (_mode == SERIAL && _getBit(ggMmio[5].data, 5)) {
        transfer.receiveBuffer <<= 1;
        if(value) _setBit(transfer.receiveBuffer, 0);

        if (++transfer.receiveCount == 8) {
            transfer.receiveCount = 0;
            ggMmio[4].data = transfer.receiveBuffer;
            _setBit(ggMmio[5].data, 1);
            if ( _getBit(ggMmio[5].data, 3) ) {
                if(transfer.serialNmi) {
                    transfer.raiseNmi = true;
                    transfer.serialNmi = false;
                }
            }
        }
    }

    if (bit == 6 && !isOutputBit(bit)) {
        if (value != _getBit(ggMmio[1].data, 6) ) {
            if(transfer.parallelNmi) {
                transfer.parallelNmi = false;
                transfer.raiseNmi = true;
            }
        }
    }

    //if (!isOutputBit(bit)) {
        if (value) {
            _setBit(ggMmio[1].data, bit);
        } else {
            _resBit(ggMmio[1].data, bit);
        }
    //}
}

bool Gear2Gear::isOutputBit(u8 bit) {
    if (bit == 4 && _getBit(ggMmio[5].data, 4)) {
        return true;
    }
    if (bit == 5 && _getBit(ggMmio[5].data, 5)) {
        return false;
    }
    if (_getBit(ggMmio[2].data, bit) ) return false;
    return true;
}

void Gear2Gear::updateReg(u8 reg, u8 pos, u8 value) {
    bool val = _getBit(value, pos);
    if (val) {
        _setBit(ggMmio[reg].data, pos);
    } else {
        _resBit(ggMmio[reg].data, pos);
    }
}

void Gear2Gear::write_port_GG_0x1(u8 value) {
    if (isOutputBit(0)) { //output
        updateReg(1, 0, value);
    } if (isOutputBit(1)) { //output
        updateReg(1, 1, value);
    } if (isOutputBit(2)) { //output
        updateReg(1, 2, value);
    } if (isOutputBit(3)) { //output
        updateReg(1, 3, value);
    } if (isOutputBit(4)) { //output
        updateReg(1, 4, value);
    } if (isOutputBit(5)) { //output
        updateReg(1, 5, value);
    } if (isOutputBit(6)) { //output
        updateReg(1, 6, value);
    }
    updateReg(1, 7, value);
    transfer.delayP = 30;
}

void Gear2Gear::write_port_GG_0x2(u8 value) {
    bool oldNint =  _getBit(ggMmio[2].data, 7);
    bool curNint = _getBit(value, 7);
    if (curNint < oldNint) {
        transfer.parallelNmi = true;
    }
    ggMmio[2].data = value;
}

void Gear2Gear::write_port_GG_0x3(u8 value) {
    ggMmio[3].data = value;

    if (_getBit(ggMmio[5].data, 4)) {
        if (transfer.sendCount == 0) {
            transfer.sendBuffer = value;
            transfer.sendCount = 8;
        } else {
            transfer.bufferFill = true;
        }

        if (!transfer.delayS) {
            transfer.delayS = transfer.speed;
        }
        if (!sys->emulateG2G()) {
            _resBit(ggMmio[5].data, 0);
        } else {
            _setBit(ggMmio[5].data, 0);
        }
    }
}

void Gear2Gear::write_port_GG_0x4(u8 value) {
    //read only
}

void Gear2Gear::write_port_GG_0x5(u8 value) {
    updateReg(5, 3, value);
    updateReg(5, 4, value);
    updateReg(5, 5, value);
    updateReg(5, 6, value);
    updateReg(5, 7, value);

    u8 selector = value >> 6;
    switch (selector & 3) {
        case 0: transfer.speed = B4800; break;
        case 1: transfer.speed = B2400; break;
        case 2: transfer.speed = B1200; break;
        case 3: transfer.speed = B300; break;
    }
}

bool Gear2Gear::isNmi() {
    bool bufferNmi = transfer.raiseNmi;
    transfer.raiseNmi = false;
    return bufferNmi;
}
