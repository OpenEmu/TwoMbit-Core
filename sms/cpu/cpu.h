
#ifndef CPU_H
#define CPU_H

#include "dataTypes.h"
#include "../../chips/z80/z80.h"
#include "../input/input.h"
#include "../processor.h"
#include "../mem.h"

class _System;
class Bus;
class Vdp;
class SN76489;
class YM2413;
class Gear2Gear;
class Serializer;

class Cpu : public Core_z80, public Mmio, public Processor {
	_System* sys;
	Vdp& vdp;
	Bus& bus;
	SN76489& sn76489;
    YM2413& ym2413;
	_Input& input;
	Gear2Gear& gear2gear;
    i32* clockVdp;
    i32* clockSn;
    i32* clockYm;
    i32* clockG2G;
	u16 vDisplayPos;
	bool oldPause;

	static Cpu* instance1;
	static Cpu* instance2;

	u16 vcounter;
	u16 hcounterMCL;
    bool _carry;

	u8 port_3f;
	u8 port_dc;
	u8 port_dd;

	u8 read_port_3F();
	u8 read_port_DC();
	u8 read_port_DD();

	void write_port_3F(u8 value);
	void write_port_DC(u8 value);
	void write_port_DD(u8 value);

	u8 read_port_7E();
	u8 read_port_7F();
	void write_port_7E(u8 value);
	void write_port_7F(u8 value);

	u8 read_port_GG_0x0();
	void write_port_GG_0x0(u8 value);

	u8 hcountBuffer;

	void updateDisplayVPos();

	u8 doSetRes(bool set, u8 pos, u8 val) {
		return set ? doSet(pos, val) : doRes(pos, val);
	}

	void addclocks(u8 cycles);
    void updateCustomInput();

    bool checkIrq();
    bool checkNmi();

	class Interrupt {
    public:
		Cpu* cpu;
		Vdp* vdp;
		_System* sys;

		bool pin_low;
		bool enabled;
		bool _register;

		virtual void poll() = 0;

		void init() {
            pin_low = _register = false;
			enabled = false;
		}

		void resetPin() {
			pin_low = false;
		}

		bool isRegister() {
			return _register;
		}

		void resetRegister() {
			_register = false;
		}

		void activate(bool state) {
			enabled = state;

			if (pin_low ) {
				if (enabled) {
                    _register = true;
				} else {
                    _register = false;
				}
			}
		}

		Interrupt(Cpu* cpu, Vdp* vdp, _System* sys) : cpu(cpu), vdp(vdp), sys(sys)  { init(); }
	};

	class Nmi : public Interrupt {
	public:
		void poll();
		_Input* input;
		Nmi(Cpu* cpu, Vdp* vdp, _System* sys, _Input* input) : Interrupt(cpu, vdp, sys), input(input) { }
		void init() {
			Interrupt::init();
		}
	};

	class FrameIrq : public Interrupt {
	public:
		void poll();
		FrameIrq(Cpu* cpu, Vdp* vdp, _System* sys) : Interrupt(cpu, vdp, sys) { }
		void init() {
			Interrupt::init();
		}
	};

	class LineIrq : public Interrupt {
    public:
		u8 counter;
		void poll();
		void init() {
			counter = 0xFF;
			Interrupt::init();
		}
		LineIrq(Cpu* cpu, Vdp* vdp, _System* sys) : Interrupt(cpu, vdp, sys) { init(); }
	};

public:
	FrameIrq frameirq;
	LineIrq lineirq;
	Nmi nmi;
    Cpu(_System* sys, Bus& bus, Vdp& vdp, SN76489& sn76489, YM2413& ym2413, _Input& input, Gear2Gear& gear2gear);
	u8 memRead(u16 addr);
	void memWrite(u16 addr, u8 value);
	u8 ioRead(u8 addr);
	void ioWrite(u8 addr, u8 data);
	void enter();
	void power();
	void reset();
	void sync(u8 cycles);
	void debugLook() {}
	void butPressed() { port_dc = 0xEF; }
	void butNotPressed() { port_dc = 0xFF; }
	u8 openBus();

    u16 getDisplayVPos() {
        return vDisplayPos;
    }
    u16 getDisplayHPos() {
        return hcounterMCL >> 1;
    }

    u8 getPort3F() {
        return port_3f;
    }
    u8 getPortDC() {
        return port_dc;
    }

    u8 getPortDD() {
        return port_dd;
    }

    void setTH(_Input::Port port, bool state);
    void latchHcount();
	u8 mmio_read(u8 pos);
	void mmio_write(u8 pos, u8 value);
	static void Enter1();
	static void Enter2();
    void syncVdp();
    void syncSn();
	void syncGear2Gear();
    void syncYm();
    _Input& getInput() { return input; }
    Bus& getBus() { return bus; }
    void serialize(Serializer& s);
};

#endif
