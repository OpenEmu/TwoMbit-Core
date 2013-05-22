
/*****************************************************************************
*	Z80 emulator
*	- optimised for thread based emulation
*	- bus accuracy
*	- easy readability (c++ templates instead of macros)
*	- covers undocumented behavior
*
*   Version 1.1
*   - checkIrq() and checkNmi() are now called one cycle before opcode edge
*       - sooner it was called on opcode edge and that is too late
*
*	Version 1.0 initial release
*	- what is missing?
*		- multi byte support for interrupt mode 0 (not needed for Sega Master System)
*		- daisy chain of nested IRQs (not needed for Sega Master System)
*****************************************************************************/

#ifndef CORE_Z80_H
#define CORE_Z80_H

#include "dataTypes.h"

//saves timing of each opcode (debug purposes)
//#define TIMING_TEST

//saves each individual opcode one times used during execution (debug purposes)
//#define OPCODE_USAGE

class Serializer;

class Core_z80 {

protected:
	#include "regs.h"
	#include "mem.h"

	//you have to implement the following pure virtual methods
	virtual u8 memRead(u16 addr) = 0;
	virtual void memWrite(u16 addr, u8 data) = 0;
	virtual u8 ioRead(u8 addr) = 0;
	virtual void ioWrite(u8 addr, u8 data) = 0;
	virtual void sync(u8 cycles) = 0; //callback of burned t-states in useful steps ( a machine cycle or smaller )
    virtual bool checkIrq() = 0; //checks for registered Irqs one cycle before opcode edge
    virtual bool checkNmi() = 0; //checks for registered Nmis one cycle before opcode edge

	//override it to get value from external device, initial open bus (last readed byte)
	virtual u8 readBus() { return openBus(); }

	//call this in a derived class for example
	void reset();
	void power();
	void nextOpcode() { //execute next opcode
		handle_interrupts();
		(this->*opcodes[fetch_opcode()])();
	}

	//core
	void (Core_z80::*opcodes[0x10000])();
	u16 opcode; //currently processed
	void handle_interrupts();
	u16 fetch_opcode();
    void lastCycle();

	enum { FETCH_FULL, IGNORE_PREFIX };
	u8 fetch_mode;
	bool stop;
    bool nmiPending;
    bool irqPending;

	struct {
		u8 imode;
		bool delay;
		bool iff1;
		bool iff2;
	} intr;

	//timinig test, meassures needed timing of an opcode to compare with reference tables
	struct {
		u16 opcode;
		int ticks;
		bool illegal;
		int alternate_ticks;
	} timingTest[1280];
	u64 tickCounter;
	void _timing_test();

	void _sync(u8 cycles) {
		#ifdef TIMING_TEST
			tickCounter += cycles;
		#endif
		sync(cycles);
	}

	//opcode usage test, lists all used opcodes in order to shrink the debug list
	void _opcode_usage();
	void _opcode_usage_init();
	u16 opcodeUsage[1280];

	static const bool parityBit[256];
	template<bool rlc, bool adjust> u8 doRL (u8 val);
	template<bool rrc, bool adjust> u8 doRR (u8 val);
	template<bool arith> u8 doSL (u8 val);
	template<bool arith> u8 doSR (u8 val);
	template<bool _8bit, bool adc> u16 doAddBase(u16 src, u16 dest);
	u16 doAdc8(u16 src, u16 dest) { return doAddBase<true, true>(src, dest); }
	u16 doAdc16(u16 src, u16 dest) { return doAddBase<false, true>(src, dest); }
	u16 doAdd8(u16 src, u16 dest) { return doAddBase<true, false>(src, dest); }
	u16 doAdd16(u16 src, u16 dest) { return doAddBase<false, false>(src, dest); }
	template<bool _8bit, bool sbc> u16 doSubBase(u16 src, u16 dest);
	u16 doSbc8(u16 src, u16 dest) { return doSubBase<true, true>(src, dest); }
	u16 doSbc16(u16 src, u16 dest) { return doSubBase<false, true>(src, dest); }
	u16 doSub8(u16 src, u16 dest) { return doSubBase<true, false>(src, dest); }
	u16 doSub16(u16 src, u16 dest) { return doSubBase<false, false>(src, dest); }
	template<bool inc> u8 doIncDec(u8 val);

	void build_optable();
	template<bool ix> void op_ddcb_fdcb();

	template<u8 regRef16, u8 regRef8, bool write, bool dis> void op_ld_1();
	template<u8 regRef16, bool write> void op_ld_2();
	template<u8 regRef8_1, u8 regRef8_2> void op_ld_3();
	template<bool ix, u8 regRef8, u8 mode, u8 bit> void op_ld_4();
	template<u16 regRef16_1, u16 regRef16_2> void op_ld_5();
	template<u8 destRef, u8 srcRef, bool _8bit, bool addr, u16 (Core_z80::*op_l)(u16 src, u16 dest)> void op_arithmetic();
	template<u8 Ref, bool addr, u8 bit> void op_bit();
	template<bool ix, u8 bit> void op_bit2() { op_bit<ix ? _IX : _IY, true, bit>(); }
	template<i8 cond> void op_call();
	void op_ccf();
	template<bool inc, bool repeat> void op_cpx();
	void op_cpl();
	void op_daa();
	template<bool inc, bool _8bit, bool addr, u8 regRef> void op_dec_inc();
	template<bool ei> void op_xI();
	void op_djnz();
	template<bool al, bool addr, u8 regRef1, u8 regRef2> void op_ex();
	template<u8 regRef> void op_in();
	template<bool repeat, bool inc> void op_inx();
	template<i8 cond, u8 regRef> void op_jp();
	template<i8 cond> void op_jr();
	void op_neg();
	template<bool repeat, bool inc> void op_outx();
	template<u8 regRef> void op_out();
	template<u8 regRef, bool addr, u8 mode, u8 adjust> void op_bitM();
	void op_rld();
	void op_rrd();
	template<u8 begin> void op_rst();
	void op_scf();

	void op_illegal();
	void op_nop();
	void op_halt();

	template<u8 regRef> void op_pop();
	template<u8 regRef> void op_push();
	template<u8 mode> void op_im();

	template<bool inc, bool repeat> void op_ldx();

	template<i8 cond> void op_ret();
	void op_reti();
	void op_retn();
	template<bool inc, bool in> void calcInOutIncDecFlags();

	void incR() {
        reg_r = (reg_r & 0x80) | ( (reg_r + 1) & 0x7F );
	}

	void exchange(u8* reg_1, u8* reg_2) {
		u8 temp = *reg_1;
		*reg_1 = *reg_2;
		*reg_2 = temp;
	}

	u8 doSet(u8 pos, u8 val) {
		val |= (1 << pos);
		return val;
	}

	u8 doRes(u8 pos, u8 val) {
		val &= ~(1 << pos);
		return val;
	}

	u16 doAnd(u16 src, u16 dest) {
		u8 result = src & dest;
		reg.b.F = 0;
		reg.bit.h = 1;
        setStandardFlags(result);
		setUndocumentedFlags(result);
		return result;
	}

	u16 doOr(u16 src, u16 dest) {
		u8 result = src | dest;
		reg.b.F = 0;
        setStandardFlags(result);
		setUndocumentedFlags(result);
		return result;
	}

	u16 doXor(u16 src, u16 dest) {
		u8 result = src ^ dest;
		reg.b.F = 0;
        setStandardFlags(result);
		setUndocumentedFlags(result);
		return result;
	}

    void setStandardFlags(u8 val, bool _pv = true) {
		reg.bit.s = (val & 0x80) != 0;
		reg.bit.z = val == 0;
		if (_pv) reg.bit.pv = !parityBit[val];
	}

	void setUndocumentedFlags(u8 val) {
		reg.bit.unused_bit3 = (val & 8) != 0;
		reg.bit.unused_bit5 = (val & 0x20) != 0;
	}

	bool getCC(u8 cond) {
		switch(cond) {
			case 0: return !reg.bit.z;
			case 1: return reg.bit.z;
			case 2: return !reg.bit.c;
			case 3: return reg.bit.c;
			case 4: return !reg.bit.pv;
			case 5: return reg.bit.pv;
			case 6: return !reg.bit.s;
			case 7: return reg.bit.s;
		}
		return true;
	}

    u8 getReg16(u8 ref) {
        return ref - 14;
    }

    enum { _B = 0, _C = 1, _D = 2, _E = 3, _H = 4, _L = 5, _F = 6, _A = 7, _IXL = 8, _IXH = 9, _IYL = 10, _IYH = 11, _R = 12, _I = 13, _AF = 14, _BC = 15, _DE = 16, _HL = 17, _IX = 18, _IY = 19, _SP = 20, _DATA = 200, _NONE = 255 };
    enum { _SET, _RESET, _RL, _RLC, _RR, _RRC, _SLA, _SLL, _SRA, _SRL };

public:
	u8 openBus() { return mdr; }
    void coreSerialize(Serializer& s);

	Core_z80() {
		regPTR[0] = &reg.b.B;
		regPTR[1] = &reg.b.C;
		regPTR[2] = &reg.b.D;
		regPTR[3] = &reg.b.E;
		regPTR[4] = &reg.b.H;
		regPTR[5] = &reg.b.L;
		regPTR[6] = &reg.b.F;
		regPTR[7] = &reg.b.A;
		regPTR[8] = &reg.b.IXl;
		regPTR[9] = &reg.b.IXh;
		regPTR[10] = &reg.b.IYl;
		regPTR[11] = &reg.b.IYh;
		regPTR[12] = &reg_r;
		regPTR[13] = &reg_i;

		regPTR16[0] = &reg.w.AF;
		regPTR16[1] = &reg.w.BC;
		regPTR16[2] = &reg.w.DE;
		regPTR16[3] = &reg.w.HL;
		regPTR16[4] = &reg.w.IX;
		regPTR16[5] = &reg.w.IY;
		regPTR16[6] = &reg.w.SP;

		fetch_mode = FETCH_FULL;

		build_optable();

		#ifdef OPCODE_USAGE
			_opcode_usage_init();
		#endif
	}
};

#endif
