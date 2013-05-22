
#include "z80.h"
#include "opcodes.t.h"

//timing self test
//generate a list of opcode timings
#ifdef TIMING_TEST
void Core_z80::_timing_test() {
	u8 pre;
	u16 pos;
	bool alternate;
	bool illegal;
	for(int i = 0; i < 0x10000; i++) {

		alternate = false;
		pre = i >> 8;
		if (i > 0xff && pre != 0xdd && pre != 0xcb && pre != 0xfd && pre != 0xed ) continue;
		if (i == 0x76 || i == 0xdd76 || i == 0xfd76) continue; //halt
		power();
AL:
		illegal = true;
		reg.w.PC = 0xC000;
        writeByte(0xC000, i <= 0xff ? i : pre, 3);
        writeByte(0xC001, i <= 0xff ? 0 : i & 0xff, 3);
		tickCounter = 0;
		for (int j = 0; j < 2; j++) {
			(this->*opcodes[fetch_opcode()])();
			if (fetch_mode == FETCH_FULL) {
				illegal = false;
				break;
			}
		}
		switch (pre) {
			case 0x0: pos = i; break;
			case 0xdd: pos = (0x01 << 8) | (i & 0xff); break;
			case 0xfd: pos = (0x02 << 8) | (i & 0xff); break;
			case 0xcb: pos = (0x03 << 8) | (i & 0xff); break;
			case 0xed: pos = (0x04 << 8) | (i & 0xff); break;
		}
		timingTest[pos].opcode = i;
		timingTest[pos].illegal = illegal;
		if (!alternate)
			timingTest[pos].ticks = tickCounter;
		else
			timingTest[pos].alternate_ticks = tickCounter;

		if (i == 0x10 && !alternate) {
			power(); alternate = true; reg.b.B = 1; goto AL;
		}
		if (i == 0x20 && !alternate) {
			power(); alternate = true; reg.bit.z = false; goto AL;
		}
		if (i == 0xC0 && !alternate) {
			power(); alternate = true; reg.bit.z = false; goto AL;
		}
		if (i == 0xC4 && !alternate) {
			power(); alternate = true; reg.bit.z = false; goto AL;
		}
	}
}
#endif

#ifdef OPCODE_USAGE
	void Core_z80::_opcode_usage_init() {
		int _size = sizeof( opcodeUsage ) / sizeof( opcodeUsage[0] );
		for (int i=0; i < _size; i++) {
			opcodeUsage[i] = 0;
		}
	}
	void Core_z80::_opcode_usage() {
		int i;
		int _size = sizeof( opcodeUsage ) / sizeof( opcodeUsage[0] );
		for (i=0; i < _size; i++) {
			if (opcodeUsage[i] == opcode) {
				return;
			}
			if (opcodeUsage[i] == 0) {
				break;
			}
		}
		opcodeUsage[i] = opcode;
	}
#endif

void Core_z80::op_illegal() {
	u8 data = opcode >> 8;
	if ( data == 0xDD || data == 0xFD ) {
		opcode &= 0x00FF;
		fetch_mode = IGNORE_PREFIX;
		intr.delay = true;
    } else {
        op_nop();
    }
}

u16 Core_z80::fetch_opcode() {

	#ifdef TIMING_TEST
		_timing_test();
	#endif

	if (fetch_mode == FETCH_FULL) {
        opcode = readPc(3, false);
		incR();
	}
	fetch_mode = FETCH_FULL;

	if (opcode == 0xED || opcode == 0xDD || opcode == 0xFD || opcode == 0xCB) {
        _sync(1);
		opcode <<= 8;
        opcode |= readPc(3, false);
		incR();
	}

	#ifdef OPCODE_USAGE
		_opcode_usage();
	#endif

	return opcode;
}

void Core_z80::lastCycle() {
    nmiPending |= checkNmi();
    irqPending |= checkIrq();
}

void Core_z80::handle_interrupts() {
	if (intr.delay) {
		intr.delay = false;
		return;
	}

    if ( nmiPending ) {
		stop = false;
		intr.iff1 = false;
		incR();
        _sync(5);
		writeStack(reg.w.PC);
		reg.w.PC = 0x0066;
	} else if ( intr.iff1 ) {
        if ( irqPending ) {
			stop = false;
			intr.iff1 = intr.iff2 = false;
			incR();
			switch(intr.imode) {
				case 0:
                    _sync(5);
					(this->*opcodes[ readBus() ])();
					break;
				case 1:
                    _sync(7);
                    writeStack(reg.w.PC);
                    reg.w.PC = 0x0038;
					break;
				case 2:
                    _sync(7);
					writeStack(reg.w.PC);
                    _sync(3);
					u16 vector = reg_i << 8;
                    _sync(3);
					vector |= readBus();
					reg.w.PC = vector;
					break;
			}
		}
	}
    nmiPending = irqPending = false;
}

void Core_z80::reset() {
	intr.iff1 = intr.iff2 = false;
	intr.delay = false;
	intr.imode = 0;
	stop = false;
    nmiPending = irqPending = false;

	reg.w.PC = reg_al.w.PC = 0;
	reg.w.SP = reg_al.w.SP = 0xDFF0;
	reg.w.AF = reg_al.w.AF = 0;
	reg.w.BC = reg_al.w.BC = 0;
	reg.w.DE = reg_al.w.DE = 0;
	reg.w.HL = reg_al.w.HL = 0;
	reg.w.IX = reg_al.w.IX = 0;
	reg.w.IY = reg_al.w.IY = 0;

	reg_r = reg_i = 0;
	fetch_mode = FETCH_FULL;
}

void Core_z80::power() {
	reset();
}

template<bool ix> void Core_z80::op_ddcb_fdcb() { //undocumented DDCB, FDCB
    _sync(1);
	displacement = readPc(5);
	switch ( readPc(3) ) {
		case 0x87: op_ld_4<ix, _A, _RESET, 0>(); break;
		case 0x8F: op_ld_4<ix, _A, _RESET, 1>(); break;
		case 0x97: op_ld_4<ix, _A, _RESET, 2>(); break;
		case 0x9F: op_ld_4<ix, _A, _RESET, 3>(); break;
		case 0xA7: op_ld_4<ix, _A, _RESET, 4>(); break;
		case 0xAF: op_ld_4<ix, _A, _RESET, 5>(); break;
		case 0xB7: op_ld_4<ix, _A, _RESET, 6>(); break;
		case 0xBF: op_ld_4<ix, _A, _RESET, 7>(); break;
		case 0x17: op_ld_4<ix, _A, _RL, 0>(); break;
		case 0x07: op_ld_4<ix, _A, _RLC, 0>(); break;
		case 0x1F: op_ld_4<ix, _A, _RR, 0>(); break;
		case 0x0F: op_ld_4<ix, _A, _RRC, 0>(); break;
		case 0xC7: op_ld_4<ix, _A, _SET, 0>(); break;
		case 0xCF: op_ld_4<ix, _A, _SET, 1>(); break;
		case 0xD7: op_ld_4<ix, _A, _SET, 2>(); break;
		case 0xDF: op_ld_4<ix, _A, _SET, 3>(); break;
		case 0xE7: op_ld_4<ix, _A, _SET, 4>(); break;
		case 0xEF: op_ld_4<ix, _A, _SET, 5>(); break;
		case 0xF7: op_ld_4<ix, _A, _SET, 6>(); break;
		case 0xFF: op_ld_4<ix, _A, _SET, 7>(); break;
		case 0x27: op_ld_4<ix, _A, _SLA, 0>(); break;
		case 0x37: op_ld_4<ix, _A, _SLL, 0>(); break;
		case 0x2F: op_ld_4<ix, _A, _SRA, 0>(); break;
		case 0x3F: op_ld_4<ix, _A, _SRL, 0>(); break;

		case 0x80: op_ld_4<ix, _B, _RESET, 0>(); break;
		case 0x88: op_ld_4<ix, _B, _RESET, 1>(); break;
		case 0x90: op_ld_4<ix, _B, _RESET, 2>(); break;
		case 0x98: op_ld_4<ix, _B, _RESET, 3>(); break;
		case 0xA0: op_ld_4<ix, _B, _RESET, 4>(); break;
		case 0xA8: op_ld_4<ix, _B, _RESET, 5>(); break;
		case 0xB0: op_ld_4<ix, _B, _RESET, 6>(); break;
		case 0xB8: op_ld_4<ix, _B, _RESET, 7>(); break;
		case 0x10: op_ld_4<ix, _B, _RL, 0>(); break;
		case 0x00: op_ld_4<ix, _B, _RLC, 0>(); break;
		case 0x18: op_ld_4<ix, _B, _RR, 0>(); break;
		case 0x08: op_ld_4<ix, _B, _RRC, 0>(); break;
		case 0xC0: op_ld_4<ix, _B, _SET, 0>(); break;
		case 0xC8: op_ld_4<ix, _B, _SET, 1>(); break;
		case 0xD0: op_ld_4<ix, _B, _SET, 2>(); break;
		case 0xD8: op_ld_4<ix, _B, _SET, 3>(); break;
		case 0xE0: op_ld_4<ix, _B, _SET, 4>(); break;
		case 0xE8: op_ld_4<ix, _B, _SET, 5>(); break;
		case 0xF0: op_ld_4<ix, _B, _SET, 6>(); break;
		case 0xF8: op_ld_4<ix, _B, _SET, 7>(); break;
		case 0x20: op_ld_4<ix, _B, _SLA, 0>(); break;
		case 0x30: op_ld_4<ix, _B, _SLL, 0>(); break;
		case 0x28: op_ld_4<ix, _B, _SRA, 0>(); break;
		case 0x38: op_ld_4<ix, _B, _SRL, 0>(); break;

		case 0x81: op_ld_4<ix, _C, _RESET, 0>(); break;
		case 0x89: op_ld_4<ix, _C, _RESET, 1>(); break;
		case 0x91: op_ld_4<ix, _C, _RESET, 2>(); break;
		case 0x99: op_ld_4<ix, _C, _RESET, 3>(); break;
		case 0xA1: op_ld_4<ix, _C, _RESET, 4>(); break;
		case 0xA9: op_ld_4<ix, _C, _RESET, 5>(); break;
		case 0xB1: op_ld_4<ix, _C, _RESET, 6>(); break;
		case 0xB9: op_ld_4<ix, _C, _RESET, 7>(); break;
		case 0x11: op_ld_4<ix, _C, _RL, 0>(); break;
		case 0x01: op_ld_4<ix, _C, _RLC, 0>(); break;
		case 0x19: op_ld_4<ix, _C, _RR, 0>(); break;
		case 0x09: op_ld_4<ix, _C, _RRC, 0>(); break;
		case 0xC1: op_ld_4<ix, _C, _SET, 0>(); break;
		case 0xC9: op_ld_4<ix, _C, _SET, 1>(); break;
		case 0xD1: op_ld_4<ix, _C, _SET, 2>(); break;
		case 0xD9: op_ld_4<ix, _C, _SET, 3>(); break;
		case 0xE1: op_ld_4<ix, _C, _SET, 4>(); break;
		case 0xE9: op_ld_4<ix, _C, _SET, 5>(); break;
		case 0xF1: op_ld_4<ix, _C, _SET, 6>(); break;
		case 0xF9: op_ld_4<ix, _C, _SET, 7>(); break;
		case 0x21: op_ld_4<ix, _C, _SLA, 0>(); break;
		case 0x31: op_ld_4<ix, _C, _SLL, 0>(); break;
		case 0x29: op_ld_4<ix, _C, _SRA, 0>(); break;
		case 0x39: op_ld_4<ix, _C, _SRL, 0>(); break;

		case 0x82: op_ld_4<ix, _D, _RESET, 0>(); break;
		case 0x8A: op_ld_4<ix, _D, _RESET, 1>(); break;
		case 0x92: op_ld_4<ix, _D, _RESET, 2>(); break;
		case 0x9A: op_ld_4<ix, _D, _RESET, 3>(); break;
		case 0xA2: op_ld_4<ix, _D, _RESET, 4>(); break;
		case 0xAA: op_ld_4<ix, _D, _RESET, 5>(); break;
		case 0xB2: op_ld_4<ix, _D, _RESET, 6>(); break;
		case 0xBA: op_ld_4<ix, _D, _RESET, 7>(); break;
		case 0x12: op_ld_4<ix, _D, _RL, 0>(); break;
		case 0x02: op_ld_4<ix, _D, _RLC, 0>(); break;
		case 0x1A: op_ld_4<ix, _D, _RR, 0>(); break;
		case 0x0A: op_ld_4<ix, _D, _RRC, 0>(); break;
		case 0xC2: op_ld_4<ix, _D, _SET, 0>(); break;
		case 0xCA: op_ld_4<ix, _D, _SET, 1>(); break;
		case 0xD2: op_ld_4<ix, _D, _SET, 2>(); break;
		case 0xDA: op_ld_4<ix, _D, _SET, 3>(); break;
		case 0xE2: op_ld_4<ix, _D, _SET, 4>(); break;
		case 0xEA: op_ld_4<ix, _D, _SET, 5>(); break;
		case 0xF2: op_ld_4<ix, _D, _SET, 6>(); break;
		case 0xFA: op_ld_4<ix, _D, _SET, 7>(); break;
		case 0x22: op_ld_4<ix, _D, _SLA, 0>(); break;
		case 0x32: op_ld_4<ix, _D, _SLL, 0>(); break;
		case 0x2A: op_ld_4<ix, _D, _SRA, 0>(); break;
		case 0x3A: op_ld_4<ix, _D, _SRL, 0>(); break;

		case 0x83: op_ld_4<ix, _E, _RESET, 0>(); break;
		case 0x8B: op_ld_4<ix, _E, _RESET, 1>(); break;
		case 0x93: op_ld_4<ix, _E, _RESET, 2>(); break;
		case 0x9B: op_ld_4<ix, _E, _RESET, 3>(); break;
		case 0xA3: op_ld_4<ix, _E, _RESET, 4>(); break;
		case 0xAB: op_ld_4<ix, _E, _RESET, 5>(); break;
		case 0xB3: op_ld_4<ix, _E, _RESET, 6>(); break;
		case 0xBB: op_ld_4<ix, _E, _RESET, 7>(); break;
		case 0x13: op_ld_4<ix, _E, _RL, 0>(); break;
		case 0x03: op_ld_4<ix, _E, _RLC, 0>(); break;
		case 0x1B: op_ld_4<ix, _E, _RR, 0>(); break;
		case 0x0B: op_ld_4<ix, _E, _RRC, 0>(); break;
		case 0xC3: op_ld_4<ix, _E, _SET, 0>(); break;
		case 0xCB: op_ld_4<ix, _E, _SET, 1>(); break;
		case 0xD3: op_ld_4<ix, _E, _SET, 2>(); break;
		case 0xDB: op_ld_4<ix, _E, _SET, 3>(); break;
		case 0xE3: op_ld_4<ix, _E, _SET, 4>(); break;
		case 0xEB: op_ld_4<ix, _E, _SET, 5>(); break;
		case 0xF3: op_ld_4<ix, _E, _SET, 6>(); break;
		case 0xFB: op_ld_4<ix, _E, _SET, 7>(); break;
		case 0x23: op_ld_4<ix, _E, _SLA, 0>(); break;
		case 0x33: op_ld_4<ix, _E, _SLL, 0>(); break;
		case 0x2B: op_ld_4<ix, _E, _SRA, 0>(); break;
		case 0x3B: op_ld_4<ix, _E, _SRL, 0>(); break;

		case 0x84: op_ld_4<ix, _H, _RESET, 0>(); break;
		case 0x8C: op_ld_4<ix, _H, _RESET, 1>(); break;
		case 0x94: op_ld_4<ix, _H, _RESET, 2>(); break;
		case 0x9C: op_ld_4<ix, _H, _RESET, 3>(); break;
		case 0xA4: op_ld_4<ix, _H, _RESET, 4>(); break;
		case 0xAC: op_ld_4<ix, _H, _RESET, 5>(); break;
		case 0xB4: op_ld_4<ix, _H, _RESET, 6>(); break;
		case 0xBC: op_ld_4<ix, _H, _RESET, 7>(); break;
		case 0x14: op_ld_4<ix, _H, _RL, 0>(); break;
		case 0x04: op_ld_4<ix, _H, _RLC, 0>(); break;
		case 0x1C: op_ld_4<ix, _H, _RR, 0>(); break;
		case 0x0C: op_ld_4<ix, _H, _RRC, 0>(); break;
		case 0xC4: op_ld_4<ix, _H, _SET, 0>(); break;
		case 0xCC: op_ld_4<ix, _H, _SET, 1>(); break;
		case 0xD4: op_ld_4<ix, _H, _SET, 2>(); break;
		case 0xDC: op_ld_4<ix, _H, _SET, 3>(); break;
		case 0xE4: op_ld_4<ix, _H, _SET, 4>(); break;
		case 0xEC: op_ld_4<ix, _H, _SET, 5>(); break;
		case 0xF4: op_ld_4<ix, _H, _SET, 6>(); break;
		case 0xFC: op_ld_4<ix, _H, _SET, 7>(); break;
		case 0x24: op_ld_4<ix, _H, _SLA, 0>(); break;
		case 0x34: op_ld_4<ix, _H, _SLL, 0>(); break;
		case 0x2C: op_ld_4<ix, _H, _SRA, 0>(); break;
		case 0x3C: op_ld_4<ix, _H, _SRL, 0>(); break;

		case 0x85: op_ld_4<ix, _L, _RESET, 0>(); break;
		case 0x8D: op_ld_4<ix, _L, _RESET, 1>(); break;
		case 0x95: op_ld_4<ix, _L, _RESET, 2>(); break;
		case 0x9D: op_ld_4<ix, _L, _RESET, 3>(); break;
		case 0xA5: op_ld_4<ix, _L, _RESET, 4>(); break;
		case 0xAD: op_ld_4<ix, _L, _RESET, 5>(); break;
		case 0xB5: op_ld_4<ix, _L, _RESET, 6>(); break;
		case 0xBD: op_ld_4<ix, _L, _RESET, 7>(); break;
		case 0x15: op_ld_4<ix, _L, _RL, 0>(); break;
		case 0x05: op_ld_4<ix, _L, _RLC, 0>(); break;
		case 0x1D: op_ld_4<ix, _L, _RR, 0>(); break;
		case 0x0D: op_ld_4<ix, _L, _RRC, 0>(); break;
		case 0xC5: op_ld_4<ix, _L, _SET, 0>(); break;
		case 0xCD: op_ld_4<ix, _L, _SET, 1>(); break;
		case 0xD5: op_ld_4<ix, _L, _SET, 2>(); break;
		case 0xDD: op_ld_4<ix, _L, _SET, 3>(); break;
		case 0xE5: op_ld_4<ix, _L, _SET, 4>(); break;
		case 0xED: op_ld_4<ix, _L, _SET, 5>(); break;
		case 0xF5: op_ld_4<ix, _L, _SET, 6>(); break;
		case 0xFD: op_ld_4<ix, _L, _SET, 7>(); break;
		case 0x25: op_ld_4<ix, _L, _SLA, 0>(); break;
		case 0x35: op_ld_4<ix, _L, _SLL, 0>(); break;
		case 0x2D: op_ld_4<ix, _L, _SRA, 0>(); break;
		case 0x3D: op_ld_4<ix, _L, _SRL, 0>(); break;

		case 0x86: op_ld_4<ix, _DATA, _RESET, 0>(); break;
		case 0x8E: op_ld_4<ix, _DATA, _RESET, 1>(); break;
		case 0x96: op_ld_4<ix, _DATA, _RESET, 2>(); break;
		case 0x9E: op_ld_4<ix, _DATA, _RESET, 3>(); break;
		case 0xA6: op_ld_4<ix, _DATA, _RESET, 4>(); break;
		case 0xAE: op_ld_4<ix, _DATA, _RESET, 5>(); break;
		case 0xB6: op_ld_4<ix, _DATA, _RESET, 6>(); break;
		case 0xBE: op_ld_4<ix, _DATA, _RESET, 7>(); break;
		case 0xC6: op_ld_4<ix, _DATA, _SET, 0>(); break;
		case 0xCE: op_ld_4<ix, _DATA, _SET, 1>(); break;
		case 0xD6: op_ld_4<ix, _DATA, _SET, 2>(); break;
		case 0xDE: op_ld_4<ix, _DATA, _SET, 3>(); break;
		case 0xE6: op_ld_4<ix, _DATA, _SET, 4>(); break;
		case 0xEE: op_ld_4<ix, _DATA, _SET, 5>(); break;
		case 0xF6: op_ld_4<ix, _DATA, _SET, 6>(); break;
		case 0xFE: op_ld_4<ix, _DATA, _SET, 7>(); break;
		case 0x16: op_ld_4<ix, _DATA, _RL, 0>(); break;
		case 0x06: op_ld_4<ix, _DATA, _RLC, 0>(); break;
		case 0x1E: op_ld_4<ix, _DATA, _RR, 0>(); break;
		case 0x0E: op_ld_4<ix, _DATA, _RRC, 0>(); break;
		case 0x3E: op_ld_4<ix, _DATA, _SRL, 0>(); break;
		case 0x2E: op_ld_4<ix, _DATA, _SRA, 0>(); break;
		case 0x36: op_ld_4<ix, _DATA, _SLL, 0>(); break;
		case 0x26: op_ld_4<ix, _DATA, _SLA, 0>(); break;

		case 0x46: op_bit2<ix, 0>(); break;
		case 0x41: op_bit2<ix, 0>(); break;
		case 0x47: op_bit2<ix, 0>(); break;
		case 0x42: op_bit2<ix, 0>(); break;
		case 0x43: op_bit2<ix, 0>(); break;
		case 0x44: op_bit2<ix, 0>(); break;
		case 0x40: op_bit2<ix, 0>(); break;
		case 0x45: op_bit2<ix, 0>(); break;

		case 0x4E: op_bit2<ix, 1>(); break;
		case 0x4B: op_bit2<ix, 1>(); break;
		case 0x4F: op_bit2<ix, 1>(); break;
		case 0x4A: op_bit2<ix, 1>(); break;
		case 0x4C: op_bit2<ix, 1>(); break;
		case 0x49: op_bit2<ix, 1>(); break;
		case 0x48: op_bit2<ix, 1>(); break;
		case 0x4D: op_bit2<ix, 1>(); break;

		case 0x56: op_bit2<ix, 2>(); break;
		case 0x53: op_bit2<ix, 2>(); break;
		case 0x52: op_bit2<ix, 2>(); break;
		case 0x51: op_bit2<ix, 2>(); break;
		case 0x54: op_bit2<ix, 2>(); break;
		case 0x55: op_bit2<ix, 2>(); break;
		case 0x57: op_bit2<ix, 2>(); break;
		case 0x50: op_bit2<ix, 2>(); break;

		case 0x5E: op_bit2<ix, 3>(); break;
		case 0x58: op_bit2<ix, 3>(); break;
		case 0x5A: op_bit2<ix, 3>(); break;
		case 0x59: op_bit2<ix, 3>(); break;
		case 0x5B: op_bit2<ix, 3>(); break;
		case 0x5F: op_bit2<ix, 3>(); break;
		case 0x5D: op_bit2<ix, 3>(); break;
		case 0x5C: op_bit2<ix, 3>(); break;

		case 0x66: op_bit2<ix, 4>(); break;
		case 0x60: op_bit2<ix, 4>(); break;
		case 0x62: op_bit2<ix, 4>(); break;
		case 0x61: op_bit2<ix, 4>(); break;
		case 0x65: op_bit2<ix, 4>(); break;
		case 0x63: op_bit2<ix, 4>(); break;
		case 0x64: op_bit2<ix, 4>(); break;
		case 0x67: op_bit2<ix, 4>(); break;

		case 0x6E: op_bit2<ix, 5>(); break;
		case 0x69: op_bit2<ix, 5>(); break;
		case 0x6D: op_bit2<ix, 5>(); break;
		case 0x6F: op_bit2<ix, 5>(); break;
		case 0x68: op_bit2<ix, 5>(); break;
		case 0x6C: op_bit2<ix, 5>(); break;
		case 0x6B: op_bit2<ix, 5>(); break;
		case 0x6A: op_bit2<ix, 5>(); break;

		case 0x76: op_bit2<ix, 6>(); break;
		case 0x77: op_bit2<ix, 6>(); break;
		case 0x75: op_bit2<ix, 6>(); break;
		case 0x74: op_bit2<ix, 6>(); break;
		case 0x73: op_bit2<ix, 6>(); break;
		case 0x71: op_bit2<ix, 6>(); break;
		case 0x70: op_bit2<ix, 6>(); break;
		case 0x72: op_bit2<ix, 6>(); break;

		case 0x7E: op_bit2<ix, 7>(); break;
		case 0x7B: op_bit2<ix, 7>(); break;
		case 0x79: op_bit2<ix, 7>(); break;
		case 0x7F: op_bit2<ix, 7>(); break;
		case 0x7C: op_bit2<ix, 7>(); break;
		case 0x7A: op_bit2<ix, 7>(); break;
		case 0x7D: op_bit2<ix, 7>(); break;
		case 0x78: op_bit2<ix, 7>(); break;
	}
}

void Core_z80::build_optable() {
	for (int x=0; x <= 0xFFFF; x++) {
		opcodes[x] = &Core_z80::op_illegal;
	}

	opcodes[0x02] = &Core_z80::op_ld_1<_BC, _A, true, false>;		//ok
	opcodes[0x12] = &Core_z80::op_ld_1<_DE, _A, true, false>;
	opcodes[0x77] = &Core_z80::op_ld_1<_HL, _A, true, false>;
	opcodes[0x70] = &Core_z80::op_ld_1<_HL, _B, true, false>;
	opcodes[0x71] = &Core_z80::op_ld_1<_HL, _C, true, false>;
	opcodes[0x72] = &Core_z80::op_ld_1<_HL, _D, true, false>;
	opcodes[0x73] = &Core_z80::op_ld_1<_HL, _E, true, false>;
	opcodes[0x74] = &Core_z80::op_ld_1<_HL, _H, true, false>;
	opcodes[0x75] = &Core_z80::op_ld_1<_HL, _L, true, false>;
	opcodes[0x36] = &Core_z80::op_ld_1<_HL, _DATA, true, false>;
	opcodes[0xDD77] = &Core_z80::op_ld_1<_IX, _A, true, true>;
	opcodes[0xDD70] = &Core_z80::op_ld_1<_IX, _B, true, true>;
	opcodes[0xDD71] = &Core_z80::op_ld_1<_IX, _C, true, true>;
	opcodes[0xDD72] = &Core_z80::op_ld_1<_IX, _D, true, true>;
	opcodes[0xDD73] = &Core_z80::op_ld_1<_IX, _E, true, true>;
	opcodes[0xDD74] = &Core_z80::op_ld_1<_IX, _H, true, true>;
	opcodes[0xDD75] = &Core_z80::op_ld_1<_IX, _L, true, true>;
	opcodes[0xDD36] = &Core_z80::op_ld_1<_IX, _DATA, true, true>;
	opcodes[0xFD77] = &Core_z80::op_ld_1<_IY, _A, true, true>;			//ok
	opcodes[0xFD70] = &Core_z80::op_ld_1<_IY, _B, true, true>;
	opcodes[0xFD71] = &Core_z80::op_ld_1<_IY, _C, true, true>;
	opcodes[0xFD72] = &Core_z80::op_ld_1<_IY, _D, true, true>;
	opcodes[0xFD73] = &Core_z80::op_ld_1<_IY, _E, true, true>;
	opcodes[0xFD74] = &Core_z80::op_ld_1<_IY, _H, true, true>;			//ok
	opcodes[0xFD75] = &Core_z80::op_ld_1<_IY, _L, true, true>;			//ok
	opcodes[0xFD36] = &Core_z80::op_ld_1<_IY, _DATA, true, true>;
	opcodes[0x32] = &Core_z80::op_ld_1<_DATA, _A, true, false>;

	opcodes[0xED43] = &Core_z80::op_ld_2<_BC, true>;
	opcodes[0xED53] = &Core_z80::op_ld_2<_DE, true>;
	opcodes[0xED63] = &Core_z80::op_ld_2<_HL, true>;
	opcodes[0x22] = &Core_z80::op_ld_2<_HL, true>;
	opcodes[0xDD22] = &Core_z80::op_ld_2<_IX, true>;
	opcodes[0xFD22] = &Core_z80::op_ld_2<_IY, true>;
	opcodes[0xED73] = &Core_z80::op_ld_2<_SP, true>;

	opcodes[0x0A] = &Core_z80::op_ld_1<_BC, _A, false, false>;
	opcodes[0x1A] = &Core_z80::op_ld_1<_DE, _A, false, false>;
	opcodes[0x7E] = &Core_z80::op_ld_1<_HL, _A, false, false>;
	opcodes[0xDD7E] = &Core_z80::op_ld_1<_IX, _A, false, true>;
	opcodes[0xFD7E] = &Core_z80::op_ld_1<_IY, _A, false, true>;
	opcodes[0x3A] = &Core_z80::op_ld_1<_DATA, _A, false, false>;

	opcodes[0x7F] = &Core_z80::op_ld_3<_A, _A>;
	opcodes[0x78] = &Core_z80::op_ld_3<_A, _B>;
	opcodes[0x79] = &Core_z80::op_ld_3<_A, _C>;
	opcodes[0x7A] = &Core_z80::op_ld_3<_A, _D>;
	opcodes[0x7B] = &Core_z80::op_ld_3<_A, _E>;
	opcodes[0x7C] = &Core_z80::op_ld_3<_A, _H>;
	opcodes[0xED57] = &Core_z80::op_ld_3<_A, _I>;		//ok
	opcodes[0xDD7C] = &Core_z80::op_ld_3<_A, _IXH>;
	opcodes[0xDD7D] = &Core_z80::op_ld_3<_A, _IXL>;
	opcodes[0xFD7C] = &Core_z80::op_ld_3<_A, _IYH>;
	opcodes[0xFD7D] = &Core_z80::op_ld_3<_A, _IYL>;
	opcodes[0x7D] = &Core_z80::op_ld_3<_A, _L>;
	opcodes[0x3E] = &Core_z80::op_ld_3<_A, _DATA>;
	opcodes[0xED5F] = &Core_z80::op_ld_3<_A, _R>;

	opcodes[0xDDCB] = &Core_z80::op_ddcb_fdcb<true>;
	opcodes[0xFDCB] = &Core_z80::op_ddcb_fdcb<false>;

	opcodes[0x46] = &Core_z80::op_ld_1<_HL, _B, false, false>;
	opcodes[0xDD46] = &Core_z80::op_ld_1<_IX, _B, false, true>;
	opcodes[0xFD46] = &Core_z80::op_ld_1<_IY, _B, false, true>;
	opcodes[0x47] = &Core_z80::op_ld_3<_B, _A>;		//ok
	opcodes[0x40] = &Core_z80::op_ld_3<_B, _B>;
	opcodes[0x41] = &Core_z80::op_ld_3<_B, _C>;
	opcodes[0x42] = &Core_z80::op_ld_3<_B, _D>;
	opcodes[0x43] = &Core_z80::op_ld_3<_B, _E>;
	opcodes[0x44] = &Core_z80::op_ld_3<_B, _H>;
	opcodes[0xDD44] = &Core_z80::op_ld_3<_B, _IXH>;
	opcodes[0xDD45] = &Core_z80::op_ld_3<_B, _IXL>;
	opcodes[0xFD44] = &Core_z80::op_ld_3<_B, _IYH>;
	opcodes[0xFD45] = &Core_z80::op_ld_3<_B, _IYL>;
	opcodes[0x45] = &Core_z80::op_ld_3<_B, _L>;
	opcodes[0x06] = &Core_z80::op_ld_3<_B, _DATA>;
	opcodes[0xED4B] = &Core_z80::op_ld_2<_BC, false>;
	opcodes[0x01] = &Core_z80::op_ld_5<_BC, _DATA>;
	opcodes[0x4E] = &Core_z80::op_ld_1<_HL, _C, false, false>;
	opcodes[0xDD4E] = &Core_z80::op_ld_1<_IX, _C, false, true>;
	opcodes[0xFD4E] = &Core_z80::op_ld_1<_IY, _C, false, true>;			//ok
	opcodes[0x4F] = &Core_z80::op_ld_3<_C, _A>;
	opcodes[0x48] = &Core_z80::op_ld_3<_C, _B>;
	opcodes[0x49] = &Core_z80::op_ld_3<_C, _C>;
	opcodes[0x4A] = &Core_z80::op_ld_3<_C, _D>;
	opcodes[0x4B] = &Core_z80::op_ld_3<_C, _E>;
	opcodes[0x4C] = &Core_z80::op_ld_3<_C, _H>;
	opcodes[0xDD4C] = &Core_z80::op_ld_3<_C, _IXH>;
	opcodes[0xDD4D] = &Core_z80::op_ld_3<_C, _IXL>;
	opcodes[0xFD4C] = &Core_z80::op_ld_3<_C, _IYH>;
	opcodes[0xFD4D] = &Core_z80::op_ld_3<_C, _IYL>;
	opcodes[0x4D] = &Core_z80::op_ld_3<_C, _L>;
	opcodes[0x0E] = &Core_z80::op_ld_3<_C, _DATA>;
	opcodes[0x56] = &Core_z80::op_ld_1<_HL, _D, false, false>;
	opcodes[0xDD56] = &Core_z80::op_ld_1<_IX, _D, false, true>;
	opcodes[0xFD56] = &Core_z80::op_ld_1<_IY, _D, false, true>;
	opcodes[0x57] = &Core_z80::op_ld_3<_D, _A>;
	opcodes[0x50] = &Core_z80::op_ld_3<_D, _B>;
	opcodes[0x51] = &Core_z80::op_ld_3<_D, _C>;
	opcodes[0x52] = &Core_z80::op_ld_3<_D, _D>;
	opcodes[0x53] = &Core_z80::op_ld_3<_D, _E>;
	opcodes[0x54] = &Core_z80::op_ld_3<_D, _H>;
	opcodes[0xDD54] = &Core_z80::op_ld_3<_D, _IXH>;
	opcodes[0xDD55] = &Core_z80::op_ld_3<_D, _IXL>;
	opcodes[0xFD54] = &Core_z80::op_ld_3<_D, _IYH>;
	opcodes[0xFD55] = &Core_z80::op_ld_3<_D, _IYL>;
	opcodes[0x55] = &Core_z80::op_ld_3<_D, _L>;
	opcodes[0x16] = &Core_z80::op_ld_3<_D, _DATA>;
	opcodes[0xED5B] = &Core_z80::op_ld_2<_DE, false>;
	opcodes[0x11] = &Core_z80::op_ld_5<_DE, _DATA>;
	opcodes[0x5E] = &Core_z80::op_ld_1<_HL, _E, false, false>;
	opcodes[0xDD5E] = &Core_z80::op_ld_1<_IX, _E, false, true>;
	opcodes[0xFD5E] = &Core_z80::op_ld_1<_IY, _E, false, true>;
	opcodes[0x5F] = &Core_z80::op_ld_3<_E, _A>;
	opcodes[0x58] = &Core_z80::op_ld_3<_E, _B>;
	opcodes[0x59] = &Core_z80::op_ld_3<_E, _C>;
	opcodes[0x5A] = &Core_z80::op_ld_3<_E, _D>;
	opcodes[0x5B] = &Core_z80::op_ld_3<_E, _E>;
	opcodes[0x5C] = &Core_z80::op_ld_3<_E, _H>;
	opcodes[0xDD5C] = &Core_z80::op_ld_3<_E, _IXH>;
	opcodes[0xDD5D] = &Core_z80::op_ld_3<_E, _IXL>;
	opcodes[0xFD5C] = &Core_z80::op_ld_3<_E, _IYH>;
	opcodes[0xFD5D] = &Core_z80::op_ld_3<_E, _IYL>;
	opcodes[0x5D] = &Core_z80::op_ld_3<_E, _L>;
	opcodes[0x1E] = &Core_z80::op_ld_3<_E, _DATA>;
	opcodes[0x66] = &Core_z80::op_ld_1<_HL, _H, false, false>;
	opcodes[0xDD66] = &Core_z80::op_ld_1<_IX, _H, false, true>;
	opcodes[0xFD66] = &Core_z80::op_ld_1<_IY, _H, false, true>;		//ok
	opcodes[0x67] = &Core_z80::op_ld_3<_H, _A>;
	opcodes[0x60] = &Core_z80::op_ld_3<_H, _B>;
	opcodes[0x61] = &Core_z80::op_ld_3<_H, _C>;
	opcodes[0x62] = &Core_z80::op_ld_3<_H, _D>;
	opcodes[0x63] = &Core_z80::op_ld_3<_H, _E>;
	opcodes[0x64] = &Core_z80::op_ld_3<_H, _H>;
	opcodes[0x65] = &Core_z80::op_ld_3<_H, _L>;
	opcodes[0x26] = &Core_z80::op_ld_3<_H, _DATA>;
	opcodes[0xED6B] = &Core_z80::op_ld_2<_HL, false>;
	opcodes[0x2A] = &Core_z80::op_ld_2<_HL, false>;
	opcodes[0x21] = &Core_z80::op_ld_5<_HL, _DATA>;
	opcodes[0xED47] = &Core_z80::op_ld_3<_I, _A>;
	opcodes[0xDD2A] = &Core_z80::op_ld_2<_IX, false>;
	opcodes[0xDD21] = &Core_z80::op_ld_5<_IX, _DATA>;
	opcodes[0xDD67] = &Core_z80::op_ld_3<_IXH, _A>;
	opcodes[0xDD60] = &Core_z80::op_ld_3<_IXH, _B>;
	opcodes[0xDD61] = &Core_z80::op_ld_3<_IXH, _C>;
	opcodes[0xDD62] = &Core_z80::op_ld_3<_IXH, _D>;
	opcodes[0xDD63] = &Core_z80::op_ld_3<_IXH, _E>;
	opcodes[0xDD64] = &Core_z80::op_ld_3<_IXH, _IXH>;
	opcodes[0xDD65] = &Core_z80::op_ld_3<_IXH, _IXL>;
	opcodes[0xDD26] = &Core_z80::op_ld_3<_IXH, _DATA>;
	opcodes[0xDD6F] = &Core_z80::op_ld_3<_IXL, _A>;
	opcodes[0xDD68] = &Core_z80::op_ld_3<_IXL, _B>;
	opcodes[0xDD69] = &Core_z80::op_ld_3<_IXL, _C>;
	opcodes[0xDD6A] = &Core_z80::op_ld_3<_IXL, _D>;
	opcodes[0xDD6B] = &Core_z80::op_ld_3<_IXL, _E>;
	opcodes[0xDD6C] = &Core_z80::op_ld_3<_IXL, _IXH>;
	opcodes[0xDD6D] = &Core_z80::op_ld_3<_IXL, _IXL>;
	opcodes[0xDD2E] = &Core_z80::op_ld_3<_IXL, _DATA>;
	opcodes[0xFD2A] = &Core_z80::op_ld_2<_IY, false>;
	opcodes[0xFD21] = &Core_z80::op_ld_5<_IY, _DATA>;
	opcodes[0xFD67] = &Core_z80::op_ld_3<_IYH, _A>;
	opcodes[0xFD60] = &Core_z80::op_ld_3<_IYH, _B>;
	opcodes[0xFD61] = &Core_z80::op_ld_3<_IYH, _C>;
	opcodes[0xFD62] = &Core_z80::op_ld_3<_IYH, _D>;
	opcodes[0xFD63] = &Core_z80::op_ld_3<_IYH, _E>;
	opcodes[0xFD64] = &Core_z80::op_ld_3<_IYH, _IYH>;
	opcodes[0xFD65] = &Core_z80::op_ld_3<_IYH, _IYL>;
	opcodes[0xFD26] = &Core_z80::op_ld_3<_IYH, _DATA>;
	opcodes[0xFD6F] = &Core_z80::op_ld_3<_IYL, _A>;
	opcodes[0xFD68] = &Core_z80::op_ld_3<_IYL, _B>;
	opcodes[0xFD69] = &Core_z80::op_ld_3<_IYL, _C>;
	opcodes[0xFD6A] = &Core_z80::op_ld_3<_IYL, _D>;
	opcodes[0xFD6B] = &Core_z80::op_ld_3<_IYL, _E>;
	opcodes[0xFD6C] = &Core_z80::op_ld_3<_IYL, _IYH>;
	opcodes[0xFD6D] = &Core_z80::op_ld_3<_IYL, _IYL>;
	opcodes[0xFD2E] = &Core_z80::op_ld_3<_IYL, _DATA>;
	opcodes[0x6E] = &Core_z80::op_ld_1<_HL, _L, false, false>;
	opcodes[0xDD6E] = &Core_z80::op_ld_1<_IX, _L, false, true>;
	opcodes[0xFD6E] = &Core_z80::op_ld_1<_IY, _L, false, true>;
	opcodes[0x6F] = &Core_z80::op_ld_3<_L, _A>;
	opcodes[0x68] = &Core_z80::op_ld_3<_L, _B>;
	opcodes[0x69] = &Core_z80::op_ld_3<_L, _C>;
	opcodes[0x6A] = &Core_z80::op_ld_3<_L, _D>;
	opcodes[0x6B] = &Core_z80::op_ld_3<_L, _E>;
	opcodes[0x6C] = &Core_z80::op_ld_3<_L, _H>;
	opcodes[0x6D] = &Core_z80::op_ld_3<_L, _L>;
	opcodes[0x2E] = &Core_z80::op_ld_3<_L, _DATA>;
	opcodes[0xED4F] = &Core_z80::op_ld_3<_R, _A>;
	opcodes[0xED7B] = &Core_z80::op_ld_2<_SP, false>;
	opcodes[0xF9] = &Core_z80::op_ld_5<_SP, _HL>;
	opcodes[0xDDF9] = &Core_z80::op_ld_5<_SP, _IX>;
	opcodes[0xFDF9] = &Core_z80::op_ld_5<_SP, _IY>;
	opcodes[0x31] = &Core_z80::op_ld_5<_SP, _DATA>;

	opcodes[0x8E] = &Core_z80::op_arithmetic<_A, _HL, true, true, &Core_z80::doAdc8>;
	opcodes[0xDD8E] = &Core_z80::op_arithmetic<_A, _IX, true, true, &Core_z80::doAdc8>;
	opcodes[0xFD8E] = &Core_z80::op_arithmetic<_A, _IY, true, true, &Core_z80::doAdc8>;
	opcodes[0x8F] = &Core_z80::op_arithmetic<_A, _A, true, false, &Core_z80::doAdc8>;
	opcodes[0x88] = &Core_z80::op_arithmetic<_A, _B, true, false, &Core_z80::doAdc8>;
	opcodes[0x89] = &Core_z80::op_arithmetic<_A, _C, true, false, &Core_z80::doAdc8>;
	opcodes[0x8A] = &Core_z80::op_arithmetic<_A, _D, true, false, &Core_z80::doAdc8>;
	opcodes[0x8B] = &Core_z80::op_arithmetic<_A, _E, true, false, &Core_z80::doAdc8>;
	opcodes[0x8C] = &Core_z80::op_arithmetic<_A, _H, true, false, &Core_z80::doAdc8>;
	opcodes[0xDD8C] = &Core_z80::op_arithmetic<_A, _IXH, true, false, &Core_z80::doAdc8>;
	opcodes[0xDD8D] = &Core_z80::op_arithmetic<_A, _IXL, true, false, &Core_z80::doAdc8>;
	opcodes[0xFD8C] = &Core_z80::op_arithmetic<_A, _IYH, true, false, &Core_z80::doAdc8>;
	opcodes[0xFD8D] = &Core_z80::op_arithmetic<_A, _IYL, true, false, &Core_z80::doAdc8>;
	opcodes[0x8D] = &Core_z80::op_arithmetic<_A, _L, true, false, &Core_z80::doAdc8>;
	opcodes[0xCE] = &Core_z80::op_arithmetic<_A, _DATA, true, false, &Core_z80::doAdc8>;
	opcodes[0xED4A] = &Core_z80::op_arithmetic<_HL, _BC, false, false, &Core_z80::doAdc16>;
	opcodes[0xED5A] = &Core_z80::op_arithmetic<_HL, _DE, false, false, &Core_z80::doAdc16>;
	opcodes[0xED6A] = &Core_z80::op_arithmetic<_HL, _HL, false, false, &Core_z80::doAdc16>;
	opcodes[0xED7A] = &Core_z80::op_arithmetic<_HL, _SP, false, false, &Core_z80::doAdc16>;
	opcodes[0x86] = &Core_z80::op_arithmetic<_A, _HL, true, true, &Core_z80::doAdd8>;
	opcodes[0xDD86] = &Core_z80::op_arithmetic<_A, _IX, true, true, &Core_z80::doAdd8>;
	opcodes[0xFD86] = &Core_z80::op_arithmetic<_A, _IY, true, true, &Core_z80::doAdd8>;
	opcodes[0x87] = &Core_z80::op_arithmetic<_A, _A, true, false, &Core_z80::doAdd8>;
	opcodes[0x80] = &Core_z80::op_arithmetic<_A, _B, true, false, &Core_z80::doAdd8>;
	opcodes[0x81] = &Core_z80::op_arithmetic<_A, _C, true, false, &Core_z80::doAdd8>;				//ok
	opcodes[0x82] = &Core_z80::op_arithmetic<_A, _D, true, false, &Core_z80::doAdd8>;
	opcodes[0x83] = &Core_z80::op_arithmetic<_A, _E, true, false, &Core_z80::doAdd8>;
	opcodes[0x84] = &Core_z80::op_arithmetic<_A, _H, true, false, &Core_z80::doAdd8>;
	opcodes[0xDD84] = &Core_z80::op_arithmetic<_A, _IXH, true, false, &Core_z80::doAdd8>;
	opcodes[0xDD85] = &Core_z80::op_arithmetic<_A, _IXL, true, false, &Core_z80::doAdd8>;
	opcodes[0xFD84] = &Core_z80::op_arithmetic<_A, _IYH, true, false, &Core_z80::doAdd8>;
	opcodes[0xFD85] = &Core_z80::op_arithmetic<_A, _IYL, true, false, &Core_z80::doAdd8>;
	opcodes[0x85] = &Core_z80::op_arithmetic<_A, _L, true, false, &Core_z80::doAdd8>;
	opcodes[0xC6] = &Core_z80::op_arithmetic<_A, _DATA, true, false, &Core_z80::doAdd8>;
	opcodes[0x09] = &Core_z80::op_arithmetic<_HL, _BC, false, false, &Core_z80::doAdd16>;
	opcodes[0x19] = &Core_z80::op_arithmetic<_HL, _DE, false, false, &Core_z80::doAdd16>;
	opcodes[0x29] = &Core_z80::op_arithmetic<_HL, _HL, false, false, &Core_z80::doAdd16>;
	opcodes[0x39] = &Core_z80::op_arithmetic<_HL, _SP, false, false, &Core_z80::doAdd16>;
	opcodes[0xDD09] = &Core_z80::op_arithmetic<_IX, _BC, false, false, &Core_z80::doAdd16>;
	opcodes[0xDD19] = &Core_z80::op_arithmetic<_IX, _DE, false, false, &Core_z80::doAdd16>;
	opcodes[0xDD29] = &Core_z80::op_arithmetic<_IX, _IX, false, false, &Core_z80::doAdd16>;
	opcodes[0xDD39] = &Core_z80::op_arithmetic<_IX, _SP, false, false, &Core_z80::doAdd16>;
	opcodes[0xFD09] = &Core_z80::op_arithmetic<_IY, _BC, false, false, &Core_z80::doAdd16>;
	opcodes[0xFD19] = &Core_z80::op_arithmetic<_IY, _DE, false, false, &Core_z80::doAdd16>;
	opcodes[0xFD29] = &Core_z80::op_arithmetic<_IY, _IY, false, false, &Core_z80::doAdd16>;
	opcodes[0xFD39] = &Core_z80::op_arithmetic<_IY, _SP, false, false, &Core_z80::doAdd16>;

	opcodes[0xA6] = &Core_z80::op_arithmetic<_A, _HL, true, true, &Core_z80::doAnd>;
	opcodes[0xDDA6] = &Core_z80::op_arithmetic<_A, _IX, true, true, &Core_z80::doAnd>;
	opcodes[0xFDA6] = &Core_z80::op_arithmetic<_A, _IY, true, true, &Core_z80::doAnd>;
	opcodes[0xA7] = &Core_z80::op_arithmetic<_A, _A, true, false, &Core_z80::doAnd>;
	opcodes[0xA0] = &Core_z80::op_arithmetic<_A, _B, true, false, &Core_z80::doAnd>;
	opcodes[0xA1] = &Core_z80::op_arithmetic<_A, _C, true, false, &Core_z80::doAnd>;		//ok
	opcodes[0xA2] = &Core_z80::op_arithmetic<_A, _D, true, false, &Core_z80::doAnd>;
	opcodes[0xA3] = &Core_z80::op_arithmetic<_A, _E, true, false, &Core_z80::doAnd>;
	opcodes[0xA4] = &Core_z80::op_arithmetic<_A, _H, true, false, &Core_z80::doAnd>;
	opcodes[0xDDA4] = &Core_z80::op_arithmetic<_A, _IXH, true, false, &Core_z80::doAnd>;
	opcodes[0xDDA5] = &Core_z80::op_arithmetic<_A, _IXL, true, false, &Core_z80::doAnd>;
	opcodes[0xFDA4] = &Core_z80::op_arithmetic<_A, _IYH, true, false, &Core_z80::doAnd>;
	opcodes[0xFDA5] = &Core_z80::op_arithmetic<_A, _IYL, true, false, &Core_z80::doAnd>;
	opcodes[0xA5] = &Core_z80::op_arithmetic<_A, _L, true, false, &Core_z80::doAnd>;
	opcodes[0xE6] = &Core_z80::op_arithmetic<_A, _DATA, true, false, &Core_z80::doAnd>;

	opcodes[0xB6] = &Core_z80::op_arithmetic<_A, _HL, true, true, &Core_z80::doOr>;
	opcodes[0xDDB6] = &Core_z80::op_arithmetic<_A, _IX, true, true, &Core_z80::doOr>;
	opcodes[0xFDB6] = &Core_z80::op_arithmetic<_A, _IY, true, true, &Core_z80::doOr>;
	opcodes[0xB7] = &Core_z80::op_arithmetic<_A, _A, true, false, &Core_z80::doOr>;
	opcodes[0xB0] = &Core_z80::op_arithmetic<_A, _B, true, false, &Core_z80::doOr>;
	opcodes[0xB1] = &Core_z80::op_arithmetic<_A, _C, true, false, &Core_z80::doOr>;
	opcodes[0xB2] = &Core_z80::op_arithmetic<_A, _D, true, false, &Core_z80::doOr>;
	opcodes[0xB3] = &Core_z80::op_arithmetic<_A, _E, true, false, &Core_z80::doOr>;
	opcodes[0xB4] = &Core_z80::op_arithmetic<_A, _H, true, false, &Core_z80::doOr>;
	opcodes[0xDDB4] = &Core_z80::op_arithmetic<_A, _IXH, true, false, &Core_z80::doOr>;
	opcodes[0xDDB5] = &Core_z80::op_arithmetic<_A, _IXL, true, false, &Core_z80::doOr>;
	opcodes[0xFDB4] = &Core_z80::op_arithmetic<_A, _IYH, true, false, &Core_z80::doOr>;
	opcodes[0xFDB5] = &Core_z80::op_arithmetic<_A, _IYL, true, false, &Core_z80::doOr>;
	opcodes[0xB5] = &Core_z80::op_arithmetic<_A, _L, true, false, &Core_z80::doOr>;				//ok
	opcodes[0xF6] = &Core_z80::op_arithmetic<_A, _DATA, true, false, &Core_z80::doOr>;

	opcodes[0xAE] = &Core_z80::op_arithmetic<_A, _HL, true, true, &Core_z80::doXor>;
	opcodes[0xDDAE] = &Core_z80::op_arithmetic<_A, _IX, true, true, &Core_z80::doXor>;
	opcodes[0xFDAE] = &Core_z80::op_arithmetic<_A, _IY, true, true, &Core_z80::doXor>;
	opcodes[0xAF] = &Core_z80::op_arithmetic<_A, _A, true, false, &Core_z80::doXor>;
	opcodes[0xA8] = &Core_z80::op_arithmetic<_A, _B, true, false, &Core_z80::doXor>;
	opcodes[0xA9] = &Core_z80::op_arithmetic<_A, _C, true, false, &Core_z80::doXor>;
	opcodes[0xAA] = &Core_z80::op_arithmetic<_A, _D, true, false, &Core_z80::doXor>;
	opcodes[0xAB] = &Core_z80::op_arithmetic<_A, _E, true, false, &Core_z80::doXor>;
	opcodes[0xAC] = &Core_z80::op_arithmetic<_A, _H, true, false, &Core_z80::doXor>;
	opcodes[0xDDAC] = &Core_z80::op_arithmetic<_A, _IXH, true, false, &Core_z80::doXor>;
	opcodes[0xDDAD] = &Core_z80::op_arithmetic<_A, _IXL, true, false, &Core_z80::doXor>;
	opcodes[0xFDAC] = &Core_z80::op_arithmetic<_A, _IYH, true, false, &Core_z80::doXor>;
	opcodes[0xFDAD] = &Core_z80::op_arithmetic<_A, _IYL, true, false, &Core_z80::doXor>;
	opcodes[0xAD] = &Core_z80::op_arithmetic<_A, _L, true, false, &Core_z80::doXor>;
	opcodes[0xEE] = &Core_z80::op_arithmetic<_A, _DATA, true, false, &Core_z80::doXor>;

	opcodes[0x9E] = &Core_z80::op_arithmetic<_A, _HL, true, true, &Core_z80::doSbc8>;
	opcodes[0xDD9E] = &Core_z80::op_arithmetic<_A, _IX, true, true, &Core_z80::doSbc8>;
	opcodes[0xFD9E] = &Core_z80::op_arithmetic<_A, _IY, true, true, &Core_z80::doSbc8>;
	opcodes[0x9F] = &Core_z80::op_arithmetic<_A, _A, true, false, &Core_z80::doSbc8>;
	opcodes[0x98] = &Core_z80::op_arithmetic<_A, _B, true, false, &Core_z80::doSbc8>;
	opcodes[0x99] = &Core_z80::op_arithmetic<_A, _C, true, false, &Core_z80::doSbc8>;
	opcodes[0x9A] = &Core_z80::op_arithmetic<_A, _D, true, false, &Core_z80::doSbc8>;
	opcodes[0x9B] = &Core_z80::op_arithmetic<_A, _E, true, false, &Core_z80::doSbc8>;
	opcodes[0x9C] = &Core_z80::op_arithmetic<_A, _H, true, false, &Core_z80::doSbc8>;
	opcodes[0xDD9C] = &Core_z80::op_arithmetic<_A, _IXH, true, false, &Core_z80::doSbc8>;
	opcodes[0xDD9D] = &Core_z80::op_arithmetic<_A, _IXL, true, false, &Core_z80::doSbc8>;
	opcodes[0xFD9C] = &Core_z80::op_arithmetic<_A, _IYH, true, false, &Core_z80::doSbc8>;
	opcodes[0xFD9D] = &Core_z80::op_arithmetic<_A, _IYL, true, false, &Core_z80::doSbc8>;
	opcodes[0x9D] = &Core_z80::op_arithmetic<_A, _L, true, false, &Core_z80::doSbc8>;
	opcodes[0xDE] = &Core_z80::op_arithmetic<_A, _DATA, true, false, &Core_z80::doSbc8>;
	opcodes[0xED42] = &Core_z80::op_arithmetic<_HL, _BC, false, false, &Core_z80::doSbc16>;
	opcodes[0xED52] = &Core_z80::op_arithmetic<_HL, _DE, false, false, &Core_z80::doSbc16>;
	opcodes[0xED62] = &Core_z80::op_arithmetic<_HL, _HL, false, false, &Core_z80::doSbc16>;
	opcodes[0xED72] = &Core_z80::op_arithmetic<_HL, _SP, false, false, &Core_z80::doSbc16>;

	opcodes[0x96] = &Core_z80::op_arithmetic<_A, _HL, true, true, &Core_z80::doSub8>;
	opcodes[0xDD96] = &Core_z80::op_arithmetic<_A, _IX, true, true, &Core_z80::doSub8>;
	opcodes[0xFD96] = &Core_z80::op_arithmetic<_A, _IY, true, true, &Core_z80::doSub8>;
	opcodes[0x97] = &Core_z80::op_arithmetic<_A, _A, true, false, &Core_z80::doSub8>;
	opcodes[0x90] = &Core_z80::op_arithmetic<_A, _B, true, false, &Core_z80::doSub8>;
	opcodes[0x91] = &Core_z80::op_arithmetic<_A, _C, true, false, &Core_z80::doSub8>;
	opcodes[0x92] = &Core_z80::op_arithmetic<_A, _D, true, false, &Core_z80::doSub8>;
	opcodes[0x93] = &Core_z80::op_arithmetic<_A, _E, true, false, &Core_z80::doSub8>;
	opcodes[0x94] = &Core_z80::op_arithmetic<_A, _H, true, false, &Core_z80::doSub8>;
	opcodes[0xDD94] = &Core_z80::op_arithmetic<_A, _IXH, true, false, &Core_z80::doSub8>;
	opcodes[0xDD95] = &Core_z80::op_arithmetic<_A, _IXL, true, false, &Core_z80::doSub8>;
	opcodes[0xFD94] = &Core_z80::op_arithmetic<_A, _IYH, true, false, &Core_z80::doSub8>;
	opcodes[0xFD95] = &Core_z80::op_arithmetic<_A, _IYL, true, false, &Core_z80::doSub8>;
	opcodes[0x95] = &Core_z80::op_arithmetic<_A, _L, true, false, &Core_z80::doSub8>;
	opcodes[0xD6] = &Core_z80::op_arithmetic<_A, _DATA, true, false, &Core_z80::doSub8>;

	opcodes[0xCB46] = &Core_z80::op_bit<_HL, true, 0>;
	opcodes[0xCB47] = &Core_z80::op_bit<_A, false, 0>;
	opcodes[0xCB40] = &Core_z80::op_bit<_B, false, 0>;
	opcodes[0xCB41] = &Core_z80::op_bit<_C, false, 0>;
	opcodes[0xCB42] = &Core_z80::op_bit<_D, false, 0>;
	opcodes[0xCB43] = &Core_z80::op_bit<_E, false, 0>;
	opcodes[0xCB44] = &Core_z80::op_bit<_H, false, 0>;
	opcodes[0xCB45] = &Core_z80::op_bit<_L, false, 0>;

	opcodes[0xCB4E] = &Core_z80::op_bit<_HL, true, 1>;
	opcodes[0xCB4F] = &Core_z80::op_bit<_A, false, 1>;
	opcodes[0xCB48] = &Core_z80::op_bit<_B, false, 1>;
	opcodes[0xCB49] = &Core_z80::op_bit<_C, false, 1>;
	opcodes[0xCB4A] = &Core_z80::op_bit<_D, false, 1>;
	opcodes[0xCB4B] = &Core_z80::op_bit<_E, false, 1>;
	opcodes[0xCB4C] = &Core_z80::op_bit<_H, false, 1>;
	opcodes[0xCB4D] = &Core_z80::op_bit<_L, false, 1>;

	opcodes[0xCB56] = &Core_z80::op_bit<_HL, true, 2>;
	opcodes[0xCB57] = &Core_z80::op_bit<_A, false, 2>;
	opcodes[0xCB50] = &Core_z80::op_bit<_B, false, 2>;
	opcodes[0xCB51] = &Core_z80::op_bit<_C, false, 2>;
	opcodes[0xCB52] = &Core_z80::op_bit<_D, false, 2>;
	opcodes[0xCB53] = &Core_z80::op_bit<_E, false, 2>;
	opcodes[0xCB54] = &Core_z80::op_bit<_H, false, 2>;
	opcodes[0xCB55] = &Core_z80::op_bit<_L, false, 2>;

	opcodes[0xCB5E] = &Core_z80::op_bit<_HL, true, 3>;
	opcodes[0xCB5F] = &Core_z80::op_bit<_A, false, 3>;
	opcodes[0xCB58] = &Core_z80::op_bit<_B, false, 3>;
	opcodes[0xCB59] = &Core_z80::op_bit<_C, false, 3>;
	opcodes[0xCB5A] = &Core_z80::op_bit<_D, false, 3>;
	opcodes[0xCB5B] = &Core_z80::op_bit<_E, false, 3>;
	opcodes[0xCB5C] = &Core_z80::op_bit<_H, false, 3>;
	opcodes[0xCB5D] = &Core_z80::op_bit<_L, false, 3>;

	opcodes[0xCB66] = &Core_z80::op_bit<_HL, true, 4>;
	opcodes[0xCB67] = &Core_z80::op_bit<_A, false, 4>;
	opcodes[0xCB60] = &Core_z80::op_bit<_B, false, 4>;
	opcodes[0xCB61] = &Core_z80::op_bit<_C, false, 4>;
	opcodes[0xCB62] = &Core_z80::op_bit<_D, false, 4>;
	opcodes[0xCB63] = &Core_z80::op_bit<_E, false, 4>;
	opcodes[0xCB64] = &Core_z80::op_bit<_H, false, 4>;
	opcodes[0xCB65] = &Core_z80::op_bit<_L, false, 4>;

	opcodes[0xCB6E] = &Core_z80::op_bit<_HL, true, 5>;
	opcodes[0xCB6F] = &Core_z80::op_bit<_A, false, 5>;
	opcodes[0xCB68] = &Core_z80::op_bit<_B, false, 5>;
	opcodes[0xCB69] = &Core_z80::op_bit<_C, false, 5>;
	opcodes[0xCB6A] = &Core_z80::op_bit<_D, false, 5>;
	opcodes[0xCB6B] = &Core_z80::op_bit<_E, false, 5>;
	opcodes[0xCB6C] = &Core_z80::op_bit<_H, false, 5>;
	opcodes[0xCB6D] = &Core_z80::op_bit<_L, false, 5>;

	opcodes[0xCB76] = &Core_z80::op_bit<_HL, true, 6>;
	opcodes[0xCB77] = &Core_z80::op_bit<_A, false, 6>;
	opcodes[0xCB70] = &Core_z80::op_bit<_B, false, 6>;
	opcodes[0xCB71] = &Core_z80::op_bit<_C, false, 6>;
	opcodes[0xCB72] = &Core_z80::op_bit<_D, false, 6>;
	opcodes[0xCB73] = &Core_z80::op_bit<_E, false, 6>;
	opcodes[0xCB74] = &Core_z80::op_bit<_H, false, 6>;
	opcodes[0xCB75] = &Core_z80::op_bit<_L, false, 6>;

	opcodes[0xCB7E] = &Core_z80::op_bit<_HL, true, 7>;
	opcodes[0xCB7F] = &Core_z80::op_bit<_A, false, 7>;	//ok
	opcodes[0xCB78] = &Core_z80::op_bit<_B, false, 7>;
	opcodes[0xCB79] = &Core_z80::op_bit<_C, false, 7>;
	opcodes[0xCB7A] = &Core_z80::op_bit<_D, false, 7>;
	opcodes[0xCB7B] = &Core_z80::op_bit<_E, false, 7>;
	opcodes[0xCB7C] = &Core_z80::op_bit<_H, false, 7>;
	opcodes[0xCB7D] = &Core_z80::op_bit<_L, false, 7>;

	opcodes[0xCD] = &Core_z80::op_call<-1>;
	opcodes[0xDC] = &Core_z80::op_call<3>;
	opcodes[0xFC] = &Core_z80::op_call<7>;
	opcodes[0xD4] = &Core_z80::op_call<2>;
	opcodes[0xC4] = &Core_z80::op_call<0>;
	opcodes[0xF4] = &Core_z80::op_call<6>;
	opcodes[0xEC] = &Core_z80::op_call<5>;
	opcodes[0xE4] = &Core_z80::op_call<4>;
	opcodes[0xCC] = &Core_z80::op_call<1>;

	opcodes[0x3F] = &Core_z80::op_ccf;

	opcodes[0xBE] = &Core_z80::op_arithmetic<_NONE, _HL, true, true, &Core_z80::doSub8>;
	opcodes[0xDDBE] = &Core_z80::op_arithmetic<_NONE, _IX, true, true, &Core_z80::doSub8>;
	opcodes[0xFDBE] = &Core_z80::op_arithmetic<_NONE, _IY, true, true, &Core_z80::doSub8>;
	opcodes[0xBF] = &Core_z80::op_arithmetic<_NONE, _A, true, false, &Core_z80::doSub8>;
	opcodes[0xB8] = &Core_z80::op_arithmetic<_NONE, _B, true, false, &Core_z80::doSub8>;		//ok
	opcodes[0xB9] = &Core_z80::op_arithmetic<_NONE, _C, true, false, &Core_z80::doSub8>;
	opcodes[0xBA] = &Core_z80::op_arithmetic<_NONE, _D, true, false, &Core_z80::doSub8>;
	opcodes[0xBB] = &Core_z80::op_arithmetic<_NONE, _E, true, false, &Core_z80::doSub8>;
	opcodes[0xBC] = &Core_z80::op_arithmetic<_NONE, _H, true, false, &Core_z80::doSub8>;
	opcodes[0xDDBC] = &Core_z80::op_arithmetic<_NONE, _IXH, true, false, &Core_z80::doSub8>;
	opcodes[0xDDBD] = &Core_z80::op_arithmetic<_NONE, _IXL, true, false, &Core_z80::doSub8>;
	opcodes[0xFDBC] = &Core_z80::op_arithmetic<_NONE, _IYH, true, false, &Core_z80::doSub8>;
	opcodes[0xFDBD] = &Core_z80::op_arithmetic<_NONE, _IYL, true, false, &Core_z80::doSub8>;
	opcodes[0xBD] = &Core_z80::op_arithmetic<_NONE, _L, true, false, &Core_z80::doSub8>;
	opcodes[0xFE] = &Core_z80::op_arithmetic<_NONE, _DATA, true, false, &Core_z80::doSub8>;

	opcodes[0xEDA9] = &Core_z80::op_cpx<false, false>;
	opcodes[0xEDB9] = &Core_z80::op_cpx<false, true>;
	opcodes[0xEDA1] = &Core_z80::op_cpx<true, false>;
	opcodes[0xEDB1] = &Core_z80::op_cpx<true, true>;

	opcodes[0x2F] = &Core_z80::op_cpl;
	opcodes[0x27] = &Core_z80::op_daa;

	opcodes[0x35] = &Core_z80::op_dec_inc<false, true, true, _HL>;
	opcodes[0xDD35] = &Core_z80::op_dec_inc<false, true, true, _IX>;
	opcodes[0xFD35] = &Core_z80::op_dec_inc<false, true, true, _IY>;
	opcodes[0x3D] = &Core_z80::op_dec_inc<false, true, false, _A>;
	opcodes[0x05] = &Core_z80::op_dec_inc<false, true, false, _B>;
	opcodes[0x0B] = &Core_z80::op_dec_inc<false, false, false, _BC>;
	opcodes[0x0D] = &Core_z80::op_dec_inc<false, true, false, _C>;
	opcodes[0x15] = &Core_z80::op_dec_inc<false, true, false, _D>;
	opcodes[0x1B] = &Core_z80::op_dec_inc<false, false, false, _DE>;
	opcodes[0x1D] = &Core_z80::op_dec_inc<false, true, false, _E>;
	opcodes[0x25] = &Core_z80::op_dec_inc<false, true, false, _H>;
	opcodes[0x2B] = &Core_z80::op_dec_inc<false, false, false, _HL>;
	opcodes[0xDD2B] = &Core_z80::op_dec_inc<false, false, false, _IX>;
	opcodes[0xDD25] = &Core_z80::op_dec_inc<false, true, false, _IXH>;
	opcodes[0xDD2D] = &Core_z80::op_dec_inc<false, true, false, _IXL>;
	opcodes[0xFD2B] = &Core_z80::op_dec_inc<false, false, false, _IY>;
	opcodes[0xFD25] = &Core_z80::op_dec_inc<false, true, false, _IYH>;
	opcodes[0xFD2D] = &Core_z80::op_dec_inc<false, true, false, _IYL>;
	opcodes[0x2D] = &Core_z80::op_dec_inc<false, true, false, _L>;
	opcodes[0x3B] = &Core_z80::op_dec_inc<false, false, false, _SP>;

	opcodes[0x34] = &Core_z80::op_dec_inc<true, true, true, _HL>;
	opcodes[0xDD34] = &Core_z80::op_dec_inc<true, true, true, _IX>;
	opcodes[0xFD34] = &Core_z80::op_dec_inc<true, true, true, _IY>;
	opcodes[0x3C] = &Core_z80::op_dec_inc<true, true, false, _A>;
	opcodes[0x04] = &Core_z80::op_dec_inc<true, true, false, _B>;
	opcodes[0x03] = &Core_z80::op_dec_inc<true, false, false, _BC>;
	opcodes[0x0C] = &Core_z80::op_dec_inc<true, true, false, _C>;
	opcodes[0x14] = &Core_z80::op_dec_inc<true, true, false, _D>;
	opcodes[0x13] = &Core_z80::op_dec_inc<true, false, false, _DE>;
	opcodes[0x1C] = &Core_z80::op_dec_inc<true, true, false, _E>;
	opcodes[0x24] = &Core_z80::op_dec_inc<true, true, false, _H>;
	opcodes[0x23] = &Core_z80::op_dec_inc<true, false, false, _HL>;
	opcodes[0xDD23] = &Core_z80::op_dec_inc<true, false, false, _IX>;
	opcodes[0xDD24] = &Core_z80::op_dec_inc<true, true, false, _IXH>;
	opcodes[0xDD2C] = &Core_z80::op_dec_inc<true, true, false, _IXL>;
	opcodes[0xFD23] = &Core_z80::op_dec_inc<true, false, false, _IY>;
	opcodes[0xFD24] = &Core_z80::op_dec_inc<true, true, false, _IYH>;
	opcodes[0xFD2C] = &Core_z80::op_dec_inc<true, true, false, _IYL>;
	opcodes[0x2C] = &Core_z80::op_dec_inc<true, true, false, _L>;
	opcodes[0x33] = &Core_z80::op_dec_inc<true, false, false, _SP>;

	opcodes[0xF3] = &Core_z80::op_xI<false>;
	opcodes[0xFB] = &Core_z80::op_xI<true>;

	opcodes[0x10] = &Core_z80::op_djnz;

	opcodes[0xE3] = &Core_z80::op_ex<false, true, _H, _L>;
	opcodes[0xDDE3] = &Core_z80::op_ex<false, true, _IXH, _IXL>;
	opcodes[0xFDE3] = &Core_z80::op_ex<false, true, _IYH, _IYL>;
	opcodes[0x08] = &Core_z80::op_ex<true, false, _A, _F>;
	opcodes[0xEB] = &Core_z80::op_ex<false, false, _NONE, _NONE>;
	opcodes[0xD9] = &Core_z80::op_ex<true, false, _NONE, _NONE>;	//ok

	opcodes[0x76] = &Core_z80::op_halt;

	opcodes[0xED78] = &Core_z80::op_in<_A>;
	opcodes[0xDB] = &Core_z80::op_in<_DATA>;
	opcodes[0xED40] = &Core_z80::op_in<_B>;
	opcodes[0xED48] = &Core_z80::op_in<_C>;
	opcodes[0xED50] = &Core_z80::op_in<_D>;
	opcodes[0xED58] = &Core_z80::op_in<_E>;
	opcodes[0xED70] = &Core_z80::op_in<_F>;
	opcodes[0xED60] = &Core_z80::op_in<_H>;
	opcodes[0xED68] = &Core_z80::op_in<_L>;

	opcodes[0xEDAA] = &Core_z80::op_inx<false, false>;
	opcodes[0xEDBA] = &Core_z80::op_inx<true, false>;
	opcodes[0xEDA2] = &Core_z80::op_inx<false, true>;
	opcodes[0xEDB2] = &Core_z80::op_inx<true, true>;

	opcodes[0xE9] = &Core_z80::op_jp<-1, _HL>;		//ok
	opcodes[0xDDE9] = &Core_z80::op_jp<-1, _IX>;
	opcodes[0xFDE9] = &Core_z80::op_jp<-1, _IY>;
	opcodes[0xC3] = &Core_z80::op_jp<-1, _DATA>;
	opcodes[0xDA] = &Core_z80::op_jp<3, _DATA>;
	opcodes[0xFA] = &Core_z80::op_jp<7, _DATA>;
	opcodes[0xD2] = &Core_z80::op_jp<2, _DATA>;
	opcodes[0xC2] = &Core_z80::op_jp<0, _DATA>;
	opcodes[0xF2] = &Core_z80::op_jp<6, _DATA>;
	opcodes[0xEA] = &Core_z80::op_jp<5, _DATA>;
	opcodes[0xE2] = &Core_z80::op_jp<4, _DATA>;
	opcodes[0xCA] = &Core_z80::op_jp<1, _DATA>;

	opcodes[0x18] = &Core_z80::op_jr<-1>;
	opcodes[0x38] = &Core_z80::op_jr<3>;
	opcodes[0x30] = &Core_z80::op_jr<2>;
	opcodes[0x20] = &Core_z80::op_jr<0>;
	opcodes[0x28] = &Core_z80::op_jr<1>;

	opcodes[0xEDA8] = &Core_z80::op_ldx<false, false>;
	opcodes[0xEDB8] = &Core_z80::op_ldx<false, true>;
	opcodes[0xEDA0] = &Core_z80::op_ldx<true, false>;
	opcodes[0xEDB0] = &Core_z80::op_ldx<true, true>;

	opcodes[0xED44] = &Core_z80::op_neg;
	opcodes[0xED64] = &Core_z80::op_neg;
	opcodes[0xED6C] = &Core_z80::op_neg;
	opcodes[0xED74] = &Core_z80::op_neg;
	opcodes[0xED5C] = &Core_z80::op_neg;
	opcodes[0xED7C] = &Core_z80::op_neg;
	opcodes[0xED54] = &Core_z80::op_neg;
	opcodes[0xED4C] = &Core_z80::op_neg;

	opcodes[0x00] = &Core_z80::op_nop;

	opcodes[0xEDBB] = &Core_z80::op_outx<true, false>;
	opcodes[0xEDB3] = &Core_z80::op_outx<true, true>;
	opcodes[0xED71] = &Core_z80::op_out<_NONE>;
	opcodes[0xED79] = &Core_z80::op_out<_A>;
	opcodes[0xED41] = &Core_z80::op_out<_B>;
	opcodes[0xED49] = &Core_z80::op_out<_C>;
	opcodes[0xED51] = &Core_z80::op_out<_D>;
	opcodes[0xED59] = &Core_z80::op_out<_E>;
	opcodes[0xED61] = &Core_z80::op_out<_H>;
	opcodes[0xED69] = &Core_z80::op_out<_L>;
	opcodes[0xD3] = &Core_z80::op_out<_DATA>;
	opcodes[0xEDAB] = &Core_z80::op_outx<false, false>;
	opcodes[0xEDA3] = &Core_z80::op_outx<false, true>;

	opcodes[0xF1] = &Core_z80::op_pop<_AF>;
	opcodes[0xC1] = &Core_z80::op_pop<_BC>;
	opcodes[0xD1] = &Core_z80::op_pop<_DE>;
	opcodes[0xE1] = &Core_z80::op_pop<_HL>;
	opcodes[0xDDE1] = &Core_z80::op_pop<_IX>;
	opcodes[0xFDE1] = &Core_z80::op_pop<_IY>;

	opcodes[0xF5] = &Core_z80::op_push<_AF>;
	opcodes[0xC5] = &Core_z80::op_push<_BC>;
	opcodes[0xD5] = &Core_z80::op_push<_DE>;
	opcodes[0xE5] = &Core_z80::op_push<_HL>;
	opcodes[0xDDE5] = &Core_z80::op_push<_IX>;
	opcodes[0xFDE5] = &Core_z80::op_push<_IY>;

	opcodes[0xCB86] = &Core_z80::op_bitM<_HL, true, _RESET, 0>;
	opcodes[0xCB87] = &Core_z80::op_bitM<_A, false, _RESET, 0>;
	opcodes[0xCB80] = &Core_z80::op_bitM<_B, false, _RESET, 0>;
	opcodes[0xCB81] = &Core_z80::op_bitM<_C, false, _RESET, 0>;
	opcodes[0xCB82] = &Core_z80::op_bitM<_D, false, _RESET, 0>;
	opcodes[0xCB83] = &Core_z80::op_bitM<_E, false, _RESET, 0>;
	opcodes[0xCB84] = &Core_z80::op_bitM<_H, false, _RESET, 0>;
	opcodes[0xCB85] = &Core_z80::op_bitM<_L, false, _RESET, 0>;

	opcodes[0xCB8E] = &Core_z80::op_bitM<_HL, true, _RESET, 1>;
	opcodes[0xCB8F] = &Core_z80::op_bitM<_A, false, _RESET, 1>;
	opcodes[0xCB88] = &Core_z80::op_bitM<_B, false, _RESET, 1>;
	opcodes[0xCB89] = &Core_z80::op_bitM<_C, false, _RESET, 1>;
	opcodes[0xCB8A] = &Core_z80::op_bitM<_D, false, _RESET, 1>;
	opcodes[0xCB8B] = &Core_z80::op_bitM<_E, false, _RESET, 1>;
	opcodes[0xCB8C] = &Core_z80::op_bitM<_H, false, _RESET, 1>;
	opcodes[0xCB8D] = &Core_z80::op_bitM<_L, false, _RESET, 1>;

	opcodes[0xCB96] = &Core_z80::op_bitM<_HL, true, _RESET, 2>;
	opcodes[0xCB97] = &Core_z80::op_bitM<_A, false, _RESET, 2>;
	opcodes[0xCB90] = &Core_z80::op_bitM<_B, false, _RESET, 2>;
	opcodes[0xCB91] = &Core_z80::op_bitM<_C, false, _RESET, 2>;
	opcodes[0xCB92] = &Core_z80::op_bitM<_D, false, _RESET, 2>;
	opcodes[0xCB93] = &Core_z80::op_bitM<_E, false, _RESET, 2>;
	opcodes[0xCB94] = &Core_z80::op_bitM<_H, false, _RESET, 2>;
	opcodes[0xCB95] = &Core_z80::op_bitM<_L, false, _RESET, 2>;

	opcodes[0xCB9E] = &Core_z80::op_bitM<_HL, true, _RESET, 3>;
	opcodes[0xCB9F] = &Core_z80::op_bitM<_A, false, _RESET, 3>;
	opcodes[0xCB98] = &Core_z80::op_bitM<_B, false, _RESET, 3>;
	opcodes[0xCB99] = &Core_z80::op_bitM<_C, false, _RESET, 3>;
	opcodes[0xCB9A] = &Core_z80::op_bitM<_D, false, _RESET, 3>;
	opcodes[0xCB9B] = &Core_z80::op_bitM<_E, false, _RESET, 3>;
	opcodes[0xCB9C] = &Core_z80::op_bitM<_H, false, _RESET, 3>;
	opcodes[0xCB9D] = &Core_z80::op_bitM<_L, false, _RESET, 3>;

	opcodes[0xCBA6] = &Core_z80::op_bitM<_HL, true, _RESET, 4>;
	opcodes[0xCBA7] = &Core_z80::op_bitM<_A, false, _RESET, 4>;
	opcodes[0xCBA0] = &Core_z80::op_bitM<_B, false, _RESET, 4>;
	opcodes[0xCBA1] = &Core_z80::op_bitM<_C, false, _RESET, 4>;
	opcodes[0xCBA2] = &Core_z80::op_bitM<_D, false, _RESET, 4>;
	opcodes[0xCBA3] = &Core_z80::op_bitM<_E, false, _RESET, 4>;
	opcodes[0xCBA4] = &Core_z80::op_bitM<_H, false, _RESET, 4>;
	opcodes[0xCBA5] = &Core_z80::op_bitM<_L, false, _RESET, 4>;

	opcodes[0xCBAE] = &Core_z80::op_bitM<_HL, true, _RESET, 5>;
	opcodes[0xCBAF] = &Core_z80::op_bitM<_A, false, _RESET, 5>;
	opcodes[0xCBA8] = &Core_z80::op_bitM<_B, false, _RESET, 5>;
	opcodes[0xCBA9] = &Core_z80::op_bitM<_C, false, _RESET, 5>;
	opcodes[0xCBAA] = &Core_z80::op_bitM<_D, false, _RESET, 5>;
	opcodes[0xCBAB] = &Core_z80::op_bitM<_E, false, _RESET, 5>;
	opcodes[0xCBAC] = &Core_z80::op_bitM<_H, false, _RESET, 5>;
	opcodes[0xCBAD] = &Core_z80::op_bitM<_L, false, _RESET, 5>;

	opcodes[0xCBB6] = &Core_z80::op_bitM<_HL, true, _RESET, 6>;
	opcodes[0xCBB7] = &Core_z80::op_bitM<_A, false, _RESET, 6>;
	opcodes[0xCBB0] = &Core_z80::op_bitM<_B, false, _RESET, 6>;
	opcodes[0xCBB1] = &Core_z80::op_bitM<_C, false, _RESET, 6>;
	opcodes[0xCBB2] = &Core_z80::op_bitM<_D, false, _RESET, 6>;
	opcodes[0xCBB3] = &Core_z80::op_bitM<_E, false, _RESET, 6>;
	opcodes[0xCBB4] = &Core_z80::op_bitM<_H, false, _RESET, 6>;
	opcodes[0xCBB5] = &Core_z80::op_bitM<_L, false, _RESET, 6>;

	opcodes[0xCBBE] = &Core_z80::op_bitM<_HL, true, _RESET, 7>;
	opcodes[0xCBBF] = &Core_z80::op_bitM<_A, false, _RESET, 7>;
	opcodes[0xCBB8] = &Core_z80::op_bitM<_B, false, _RESET, 7>;
	opcodes[0xCBB9] = &Core_z80::op_bitM<_C, false, _RESET, 7>;
	opcodes[0xCBBA] = &Core_z80::op_bitM<_D, false, _RESET, 7>;
	opcodes[0xCBBB] = &Core_z80::op_bitM<_E, false, _RESET, 7>;
	opcodes[0xCBBC] = &Core_z80::op_bitM<_H, false, _RESET, 7>;
	opcodes[0xCBBD] = &Core_z80::op_bitM<_L, false, _RESET, 7>;

	opcodes[0xC9] = &Core_z80::op_ret<-1>;
	opcodes[0xD8] = &Core_z80::op_ret<3>;
	opcodes[0xF8] = &Core_z80::op_ret<7>;
	opcodes[0xD0] = &Core_z80::op_ret<2>;
	opcodes[0xC0] = &Core_z80::op_ret<0>;
	opcodes[0xF0] = &Core_z80::op_ret<6>;
	opcodes[0xE8] = &Core_z80::op_ret<5>;
	opcodes[0xE0] = &Core_z80::op_ret<4>;
	opcodes[0xC8] = &Core_z80::op_ret<1>;

	opcodes[0xED4D] = &Core_z80::op_reti;
	opcodes[0xED45] = &Core_z80::op_retn;
	opcodes[0xED55] = &Core_z80::op_retn;
	opcodes[0xED65] = &Core_z80::op_retn;
	opcodes[0xED5D] = &Core_z80::op_reti;
	opcodes[0xED75] = &Core_z80::op_retn;
	opcodes[0xED6D] = &Core_z80::op_reti;
	opcodes[0xED7D] = &Core_z80::op_reti;

	opcodes[0xCB16] = &Core_z80::op_bitM<_HL, true, _RL, 1>;
	opcodes[0xCB17] = &Core_z80::op_bitM<_A, false, _RL, 1>;
	opcodes[0xCB10] = &Core_z80::op_bitM<_B, false, _RL, 1>;
	opcodes[0xCB11] = &Core_z80::op_bitM<_C, false, _RL, 1>;
	opcodes[0xCB12] = &Core_z80::op_bitM<_D, false, _RL, 1>;
	opcodes[0xCB13] = &Core_z80::op_bitM<_E, false, _RL, 1>;
	opcodes[0xCB14] = &Core_z80::op_bitM<_H, false, _RL, 1>;
	opcodes[0xCB15] = &Core_z80::op_bitM<_L, false, _RL, 1>;

	opcodes[0x17] = &Core_z80::op_bitM<_A, false, _RL, 0>;

	opcodes[0xCB06] = &Core_z80::op_bitM<_HL, true, _RLC, 1>;
	opcodes[0xCB07] = &Core_z80::op_bitM<_A, false, _RLC, 1>;
	opcodes[0xCB00] = &Core_z80::op_bitM<_B, false, _RLC, 1>;
	opcodes[0xCB01] = &Core_z80::op_bitM<_C, false, _RLC, 1>;
	opcodes[0xCB02] = &Core_z80::op_bitM<_D, false, _RLC, 1>;
	opcodes[0xCB03] = &Core_z80::op_bitM<_E, false, _RLC, 1>;
	opcodes[0xCB04] = &Core_z80::op_bitM<_H, false, _RLC, 1>;
	opcodes[0xCB05] = &Core_z80::op_bitM<_L, false, _RLC, 1>;

	opcodes[0x07] = &Core_z80::op_bitM<_A, false, _RLC, 0>;

	opcodes[0xED6F] = &Core_z80::op_rld;

	opcodes[0xCB1E] = &Core_z80::op_bitM<_HL, true, _RR, 1>;
	opcodes[0xCB1F] = &Core_z80::op_bitM<_A, false, _RR, 1>;
	opcodes[0xCB18] = &Core_z80::op_bitM<_B, false, _RR, 1>;
	opcodes[0xCB19] = &Core_z80::op_bitM<_C, false, _RR, 1>;
	opcodes[0xCB1A] = &Core_z80::op_bitM<_D, false, _RR, 1>;
	opcodes[0xCB1B] = &Core_z80::op_bitM<_E, false, _RR, 1>;
	opcodes[0xCB1C] = &Core_z80::op_bitM<_H, false, _RR, 1>;
	opcodes[0xCB1D] = &Core_z80::op_bitM<_L, false, _RR, 1>;

	opcodes[0x1F] = &Core_z80::op_bitM<_A, false, _RR, 0>;

	opcodes[0xCB0E] = &Core_z80::op_bitM<_HL, true, _RRC, 1>;
	opcodes[0xCB0F] = &Core_z80::op_bitM<_A, false, _RRC, 1>;
	opcodes[0xCB08] = &Core_z80::op_bitM<_B, false, _RRC, 1>;
	opcodes[0xCB09] = &Core_z80::op_bitM<_C, false, _RRC, 1>;
	opcodes[0xCB0A] = &Core_z80::op_bitM<_D, false, _RRC, 1>;
	opcodes[0xCB0B] = &Core_z80::op_bitM<_E, false, _RRC, 1>;
	opcodes[0xCB0C] = &Core_z80::op_bitM<_H, false, _RRC, 1>;
	opcodes[0xCB0D] = &Core_z80::op_bitM<_L, false, _RRC, 1>;

	opcodes[0x0F] = &Core_z80::op_bitM<_A, false, _RRC, 0>;			//ok

	opcodes[0xED67] = &Core_z80::op_rrd;

	opcodes[0xC7] = &Core_z80::op_rst<0>;
	opcodes[0xD7] = &Core_z80::op_rst<0x10>;
	opcodes[0xDF] = &Core_z80::op_rst<0x18>;
	opcodes[0xE7] = &Core_z80::op_rst<0x20>;
	opcodes[0xEF] = &Core_z80::op_rst<0x28>;
	opcodes[0xF7] = &Core_z80::op_rst<0x30>;
	opcodes[0xFF] = &Core_z80::op_rst<0x38>;
	opcodes[0xCF] = &Core_z80::op_rst<0x08>;

	opcodes[0x37] = &Core_z80::op_scf;

	opcodes[0xCBC6] = &Core_z80::op_bitM<_HL, true, _SET, 0>;
	opcodes[0xCBC7] = &Core_z80::op_bitM<_A, false, _SET, 0>;
	opcodes[0xCBC0] = &Core_z80::op_bitM<_B, false, _SET, 0>;
	opcodes[0xCBC1] = &Core_z80::op_bitM<_C, false, _SET, 0>;
	opcodes[0xCBC2] = &Core_z80::op_bitM<_D, false, _SET, 0>;
	opcodes[0xCBC3] = &Core_z80::op_bitM<_E, false, _SET, 0>;
	opcodes[0xCBC4] = &Core_z80::op_bitM<_H, false, _SET, 0>;
	opcodes[0xCBC5] = &Core_z80::op_bitM<_L, false, _SET, 0>;

	opcodes[0xCBCE] = &Core_z80::op_bitM<_HL, true, _SET, 1>;
	opcodes[0xCBCF] = &Core_z80::op_bitM<_A, false, _SET, 1>;
	opcodes[0xCBC8] = &Core_z80::op_bitM<_B, false, _SET, 1>;
	opcodes[0xCBC9] = &Core_z80::op_bitM<_C, false, _SET, 1>;
	opcodes[0xCBCA] = &Core_z80::op_bitM<_D, false, _SET, 1>;
	opcodes[0xCBCB] = &Core_z80::op_bitM<_E, false, _SET, 1>;
	opcodes[0xCBCC] = &Core_z80::op_bitM<_H, false, _SET, 1>;
	opcodes[0xCBCD] = &Core_z80::op_bitM<_L, false, _SET, 1>;

	opcodes[0xCBD6] = &Core_z80::op_bitM<_HL, true, _SET, 2>;
	opcodes[0xCBD7] = &Core_z80::op_bitM<_A, false, _SET, 2>;
	opcodes[0xCBD0] = &Core_z80::op_bitM<_B, false, _SET, 2>;
	opcodes[0xCBD1] = &Core_z80::op_bitM<_C, false, _SET, 2>;
	opcodes[0xCBD2] = &Core_z80::op_bitM<_D, false, _SET, 2>;
	opcodes[0xCBD3] = &Core_z80::op_bitM<_E, false, _SET, 2>;
	opcodes[0xCBD4] = &Core_z80::op_bitM<_H, false, _SET, 2>;
	opcodes[0xCBD5] = &Core_z80::op_bitM<_L, false, _SET, 2>;

	opcodes[0xCBDE] = &Core_z80::op_bitM<_HL, true, _SET, 3>;
	opcodes[0xCBDF] = &Core_z80::op_bitM<_A, false, _SET, 3>;
	opcodes[0xCBD8] = &Core_z80::op_bitM<_B, false, _SET, 3>;
	opcodes[0xCBD9] = &Core_z80::op_bitM<_C, false, _SET, 3>;
	opcodes[0xCBDA] = &Core_z80::op_bitM<_D, false, _SET, 3>;
	opcodes[0xCBDB] = &Core_z80::op_bitM<_E, false, _SET, 3>;
	opcodes[0xCBDC] = &Core_z80::op_bitM<_H, false, _SET, 3>;
	opcodes[0xCBDD] = &Core_z80::op_bitM<_L, false, _SET, 3>;

	opcodes[0xCBE6] = &Core_z80::op_bitM<_HL, true, _SET, 4>;
	opcodes[0xCBE7] = &Core_z80::op_bitM<_A, false, _SET, 4>;
	opcodes[0xCBE0] = &Core_z80::op_bitM<_B, false, _SET, 4>;
	opcodes[0xCBE1] = &Core_z80::op_bitM<_C, false, _SET, 4>;
	opcodes[0xCBE2] = &Core_z80::op_bitM<_D, false, _SET, 4>;
	opcodes[0xCBE3] = &Core_z80::op_bitM<_E, false, _SET, 4>;
	opcodes[0xCBE4] = &Core_z80::op_bitM<_H, false, _SET, 4>;
	opcodes[0xCBE5] = &Core_z80::op_bitM<_L, false, _SET, 4>;

	opcodes[0xCBEE] = &Core_z80::op_bitM<_HL, true, _SET, 5>;
	opcodes[0xCBEF] = &Core_z80::op_bitM<_A, false, _SET, 5>;
	opcodes[0xCBE8] = &Core_z80::op_bitM<_B, false, _SET, 5>;
	opcodes[0xCBE9] = &Core_z80::op_bitM<_C, false, _SET, 5>;
	opcodes[0xCBEA] = &Core_z80::op_bitM<_D, false, _SET, 5>;
	opcodes[0xCBEB] = &Core_z80::op_bitM<_E, false, _SET, 5>;
	opcodes[0xCBEC] = &Core_z80::op_bitM<_H, false, _SET, 5>;
	opcodes[0xCBED] = &Core_z80::op_bitM<_L, false, _SET, 5>;

	opcodes[0xCBF6] = &Core_z80::op_bitM<_HL, true, _SET, 6>;
	opcodes[0xCBF7] = &Core_z80::op_bitM<_A, false, _SET, 6>;
	opcodes[0xCBF0] = &Core_z80::op_bitM<_B, false, _SET, 6>;
	opcodes[0xCBF1] = &Core_z80::op_bitM<_C, false, _SET, 6>;
	opcodes[0xCBF2] = &Core_z80::op_bitM<_D, false, _SET, 6>;
	opcodes[0xCBF3] = &Core_z80::op_bitM<_E, false, _SET, 6>;
	opcodes[0xCBF4] = &Core_z80::op_bitM<_H, false, _SET, 6>;
	opcodes[0xCBF5] = &Core_z80::op_bitM<_L, false, _SET, 6>;

	opcodes[0xCBFE] = &Core_z80::op_bitM<_HL, true, _SET, 7>;
	opcodes[0xCBFF] = &Core_z80::op_bitM<_A, false, _SET, 7>;
	opcodes[0xCBF8] = &Core_z80::op_bitM<_B, false, _SET, 7>;
	opcodes[0xCBF9] = &Core_z80::op_bitM<_C, false, _SET, 7>;
	opcodes[0xCBFA] = &Core_z80::op_bitM<_D, false, _SET, 7>;
	opcodes[0xCBFB] = &Core_z80::op_bitM<_E, false, _SET, 7>;
	opcodes[0xCBFC] = &Core_z80::op_bitM<_H, false, _SET, 7>;
	opcodes[0xCBFD] = &Core_z80::op_bitM<_L, false, _SET, 7>;

	opcodes[0xCB26] = &Core_z80::op_bitM<_HL, true, _SLA, 1>;
	opcodes[0xCB27] = &Core_z80::op_bitM<_A, false, _SLA, 1>;
	opcodes[0xCB20] = &Core_z80::op_bitM<_B, false, _SLA, 1>;
	opcodes[0xCB21] = &Core_z80::op_bitM<_C, false, _SLA, 1>;
	opcodes[0xCB22] = &Core_z80::op_bitM<_D, false, _SLA, 1>;
	opcodes[0xCB23] = &Core_z80::op_bitM<_E, false, _SLA, 1>;
	opcodes[0xCB24] = &Core_z80::op_bitM<_H, false, _SLA, 1>;
	opcodes[0xCB25] = &Core_z80::op_bitM<_L, false, _SLA, 1>;

	opcodes[0xCB36] = &Core_z80::op_bitM<_HL, true, _SLL, 1>;
	opcodes[0xCB37] = &Core_z80::op_bitM<_A, false, _SLL, 1>;
	opcodes[0xCB30] = &Core_z80::op_bitM<_B, false, _SLL, 1>;
	opcodes[0xCB31] = &Core_z80::op_bitM<_C, false, _SLL, 1>;
	opcodes[0xCB32] = &Core_z80::op_bitM<_D, false, _SLL, 1>;
	opcodes[0xCB33] = &Core_z80::op_bitM<_E, false, _SLL, 1>;
	opcodes[0xCB34] = &Core_z80::op_bitM<_H, false, _SLL, 1>;
	opcodes[0xCB35] = &Core_z80::op_bitM<_L, false, _SLL, 1>;

	opcodes[0xCB2E] = &Core_z80::op_bitM<_HL, true, _SRA, 1>;
	opcodes[0xCB2F] = &Core_z80::op_bitM<_A, false, _SRA, 1>;
	opcodes[0xCB28] = &Core_z80::op_bitM<_B, false, _SRA, 1>;
	opcodes[0xCB29] = &Core_z80::op_bitM<_C, false, _SRA, 1>;
	opcodes[0xCB2A] = &Core_z80::op_bitM<_D, false, _SRA, 1>;
	opcodes[0xCB2B] = &Core_z80::op_bitM<_E, false, _SRA, 1>;
	opcodes[0xCB2C] = &Core_z80::op_bitM<_H, false, _SRA, 1>;
	opcodes[0xCB2D] = &Core_z80::op_bitM<_L, false, _SRA, 1>;

	opcodes[0xCB3E] = &Core_z80::op_bitM<_HL, true, _SRL, 1>;
	opcodes[0xCB3F] = &Core_z80::op_bitM<_A, false, _SRL, 1>;
	opcodes[0xCB38] = &Core_z80::op_bitM<_B, false, _SRL, 1>;
	opcodes[0xCB39] = &Core_z80::op_bitM<_C, false, _SRL, 1>;
	opcodes[0xCB3A] = &Core_z80::op_bitM<_D, false, _SRL, 1>;
	opcodes[0xCB3B] = &Core_z80::op_bitM<_E, false, _SRL, 1>;
	opcodes[0xCB3C] = &Core_z80::op_bitM<_H, false, _SRL, 1>;
	opcodes[0xCB3D] = &Core_z80::op_bitM<_L, false, _SRL, 1>;

	opcodes[0xED46] = &Core_z80::op_im<0>;
	opcodes[0xED6E] = &Core_z80::op_im<0>;
	opcodes[0xED4E] = &Core_z80::op_im<0>;
	opcodes[0xED66] = &Core_z80::op_im<0>;
	opcodes[0xED56] = &Core_z80::op_im<1>;
	opcodes[0xED76] = &Core_z80::op_im<1>;
	opcodes[0xED5E] = &Core_z80::op_im<2>;
	opcodes[0xED7E] = &Core_z80::op_im<2>;
}


template<bool _8bit, bool adc> u16 Core_z80::doAddBase(u16 src, u16 dest) {
	u32 result = src + dest + (adc ? u8(reg.bit.c) : 0);
	reg.bit.n = 0;

	if (_8bit) {
        setStandardFlags(u8(result), false);
		reg.bit.h = !!( (src ^ dest ^ result) & 0x10 );
		reg.bit.c = !!( result & 0x100 );
		reg.bit.pv = (dest ^ src ^ 0x80) & (dest ^ result) & 0x80;
		setUndocumentedFlags(result);
	} else {
		reg.bit.c = !!( result & 0x10000 );
		reg.bit.h = !!( (src ^ dest ^ u16(result)) & 0x1000 );
		if (adc) {
			reg.bit.pv = !!( (src ^ dest ^ 0x8000) & (dest ^ u16(result)) & 0x8000 );
			reg.bit.z = u16(result) == 0;
			reg.bit.s = u16(result) & 0x8000;
		}
		setUndocumentedFlags( (result >> 8) & 0xFF );
	}
	return result;
}

template<bool _8bit, bool sbc> u16 Core_z80::doSubBase(u16 src, u16 dest) {
	u32 result = src - ( dest + (sbc ? u8(reg.bit.c) : 0) );
	reg.bit.n = 1;

	if (_8bit) {
        setStandardFlags(u8(result), false);
		reg.bit.h = !!( (src ^ dest ^ result) & 0x10 );
		reg.bit.c = src < (dest + (sbc ? u8(reg.bit.c) : 0));
		reg.bit.pv = (src ^ dest) & (src ^ result) & 0x80;
		setUndocumentedFlags(result);
	} else { //sbc only, there is no sub16
		reg.bit.c = !!( result & 0x10000 );
		reg.bit.h = !!( (src ^ dest ^ u16(result)) & 0x1000 );
		reg.bit.pv = !!( (src ^ dest) & (src ^ u16(result)) & 0x8000 );
		reg.bit.z = u16(result) == 0;
		reg.bit.s = u16(result) & 0x8000;
		setUndocumentedFlags( (result >> 8) & 0xFF );
	}
	return result;
}


template<bool rlc, bool adjust> u8 Core_z80::doRL (u8 val) {
    bool _c = reg.bit.c;
	reg.bit.c = !! (val & 0x80);
    val <<= 1;
    val |= rlc ? (u8)reg.bit.c : (u8)_c;
	setUndocumentedFlags(val);
	reg.bit.h = reg.bit.n = 0;
    if (adjust) setStandardFlags(val);
    return val;
}

template<bool rrc, bool adjust> u8 Core_z80::doRR (u8 val) {
    bool _c = reg.bit.c;
	reg.bit.c = !! (val & 0x01);
    val >>= 1;
    val |= rrc ? u8 (reg.bit.c << 7) : u8 (_c << 7);
	setUndocumentedFlags(val);
	reg.bit.h = reg.bit.n = 0;
    if (adjust) setStandardFlags(val);
    return val;
}

template<bool arith> u8 Core_z80::doSL (u8 val) {
	reg.bit.c = !! (val & 0x80);
	val <<= 1;
	if (!arith) val |= 1;
	setUndocumentedFlags(val);
	reg.bit.h = reg.bit.n = 0;
    setStandardFlags(val);
	return val;
}

template<bool arith> u8 Core_z80::doSR (u8 val) {
	u8 _temp = val & 0x80;
	reg.bit.c = !! (val & 0x01);
	val >>= 1;
	if (arith) val |= _temp;
	setUndocumentedFlags(val);
	reg.bit.h = reg.bit.n = 0;
    setStandardFlags(val);
	return val;
}

template<bool inc> u8 Core_z80::doIncDec(u8 val) {
	if (inc) val++;
	else val--;

	reg.bit.n = inc ? 0 : 1;
	reg.bit.pv = inc ? val == 0x80 : val == 0x7F;
	reg.bit.h = inc ? (val & 0x0F ? 0 : 1) : ( (val & 0x0F) == 0x0F ? 1 : 0 );

    setStandardFlags(val, false);
	setUndocumentedFlags(val);
	return val;
}


template<bool inc, bool in> void Core_z80::calcInOutIncDecFlags() {
	setUndocumentedFlags( reg.b.B );
	reg.bit.s = (reg.b.B & 0x80) != 0;
	reg.bit.z = reg.b.B == 0;
	reg.bit.n = (data8 & 0x80) != 0;
	u16 temp;

	if (!in) {
		temp = reg.b.L + data8;
	} else {
		temp = data8 + ((reg.b.C + (inc ? 1 : -1) ) & 255);
	}
	reg.bit.pv = !parityBit[( (temp & 7) ^ reg.b.B)];
	reg.bit.c = ( temp & 0x100 ) != 0;
	reg.bit.h = reg.bit.c;
}

const bool Core_z80::parityBit[256] = {
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

