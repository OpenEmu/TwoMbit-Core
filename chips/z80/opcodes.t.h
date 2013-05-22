
template<u8 regRef16, u8 regRef8, bool write, bool dis> void Core_z80::op_ld_1() {
    _sync(1);
	if (dis) displacement = readPc(3);
	if (regRef8 == _DATA) data8 = readPc(3);
	if (regRef16 == _DATA) {
		data16 = readPc(3);
		data16 |= readPc(3) << 8;
	}

    if (dis) _sync(regRef8 == _DATA ? 2 : 5);

	if (write) writeByte(
        (regRef16 == _DATA ? data16 : *regPTR16[getReg16(regRef16)]) + (dis ? i8(displacement) : 0),
        regRef8 == _DATA ? data8 : *regPTR[regRef8], 3, true);
	else *regPTR[regRef8] = readByte(
        (regRef16 == _DATA ? data16 : *regPTR16[getReg16(regRef16)]) + (dis ? i8(displacement) : 0), 3, true);
}

template<u8 regRef16, bool write> void Core_z80::op_ld_2() {
    _sync(1);
	data16 = readPc(3);
	data16 |= readPc(3) << 8;
	if (write) {
        writeByte(data16, *regPTR16[getReg16(regRef16)] & 0xFF, 3);
        writeByte(data16 + 1, (*regPTR16[getReg16(regRef16)] >> 8) & 0xFF, 3, true);
	} else {
        *regPTR16[getReg16(regRef16)] = readByte(data16, 3);
        *regPTR16[getReg16(regRef16)] |= readByte(data16 + 1, 3, true) << 8;
	}
}

template<u8 regRef8_1, u8 regRef8_2> void Core_z80::op_ld_3() {
    bool useRI = (regRef8_2 == _I || regRef8_2 == _R || regRef8_1 == _I || regRef8_1 == _R);

    if (!useRI && regRef8_2 != _DATA) {
        lastCycle();
    }
    _sync(1);

    if (regRef8_2 == _DATA) data8 = readPc(3, true);

    *regPTR[regRef8_1] = regRef8_2 == _DATA ? data8 : *regPTR[regRef8_2];
    if (regRef8_2 == _I || regRef8_2 == _R) {
        reg.bit.s = *regPTR[regRef8_2] & 0x80;
        reg.bit.z = *regPTR[regRef8_2] == 0;
        reg.bit.h = 0;
        reg.bit.pv = intr.iff2;
        reg.bit.n = 0;
        setUndocumentedFlags(*regPTR[regRef8_1]);
    }
    if (useRI) {
        lastCycle();
        _sync(1);
    }
}

template<bool ix, u8 regRef8, u8 mode, u8 bit> void Core_z80::op_ld_4() {
	data8 = readByte((ix ? reg.w.IX : reg.w.IY) + i8(displacement), 4);
	switch ( mode ) {
		case _SET: data8 = doSet(bit, data8); break;
		case _RESET: data8 = doRes(bit, data8); break;
		case _RL: data8 = doRL<false, true> (data8); break;
		case _RLC: data8 = doRL<true, true> (data8); break;
		case _RR: data8 = doRR<false, true> (data8); break;
		case _RRC: data8 = doRR<true, true> (data8); break;
		case _SLL: data8 = doSL<false> (data8); break;
		case _SLA: data8 = doSL<true> (data8); break;
		case _SRL: data8 = doSR<false> (data8); break;
		case _SRA: data8 = doSR<true> (data8); break;
	}
	if (regRef8 != _DATA) *regPTR[regRef8] = data8;
    writeByte((ix ? reg.w.IX : reg.w.IY) + i8(displacement), data8, 3, true);
}

template<u16 regRef16_1, u16 regRef16_2> void Core_z80::op_ld_5() {
    _sync(1);
	if (regRef16_2 == _DATA) {
		data16 = readPc(3);
        data16 |= readPc(3, true) << 8;
    } else {
        _sync(1);
        lastCycle();
        _sync(1);
    }
    *regPTR16[getReg16(regRef16_1)] = regRef16_2 == _DATA ? data16 : *regPTR16[getReg16(regRef16_2)];
}

template<bool ei> void Core_z80::op_xI() {
    lastCycle();
    _sync(1);
	intr.iff1 = intr.iff2 = ei;
    if (ei) {
        intr.delay = true;
    }
}

template<u8 regRef> void Core_z80::op_pop() {
    _sync(1);
    *regPTR16[getReg16(regRef)] = readStack();
}

template<u8 regRef> void Core_z80::op_push() {
    _sync(2);
    writeStack(*regPTR16[getReg16(regRef)], true);
}

template<bool inc, bool repeat> void Core_z80::op_cpx() {
    _sync(1);
	data8 = readByte(reg.w.HL, 3);
	bool temp = reg.bit.c;
	data8 = doSub8(reg.b.A, data8);
	data8 -= reg.bit.h;
	reg.bit.unused_bit3 = (data8 & 8) != 0;
	reg.bit.unused_bit5 = (data8 & 2) != 0;
	reg.w.HL = reg.w.HL + ( inc ? 1 : -1 );
	reg.w.BC -= 1;
	reg.bit.c = temp;
	reg.bit.pv = reg.w.BC != 0;
    bool loop = (reg.w.BC != 0) && !reg.bit.z;
    _sync(4);
    if (!repeat || !loop) lastCycle();
    _sync(1);

	if (!repeat) return;
    if ( loop ) {
		reg.w.PC -= 2;
        _sync(4);
        lastCycle();
        _sync(1);
	}
}

template<bool inc, bool repeat> void Core_z80::op_ldx() {
    _sync(1);
	data8 = readByte(reg.w.HL, 3);
    writeByte(reg.w.DE, data8, 3);
	reg.w.DE +=  inc ? 1 : -1;
	reg.w.HL +=  inc ? 1 : -1;
	reg.w.BC -= 1;
	reg.bit.h = reg.bit.n = 0;
	reg.bit.pv = reg.w.BC != 0;
	data8 = reg.b.A + data8;
	reg.bit.unused_bit3 = (data8 & 8) != 0;
	reg.bit.unused_bit5 = (data8 & 2) != 0;
    bool loop = reg.w.BC != 0;
    _sync(1);
    if (!repeat || !loop) lastCycle();
    _sync(1);

	if (!repeat) return;
    if ( loop ) {
		reg.w.PC -= 2;
        _sync(4);
        lastCycle();
        _sync(1);
	}
}

template<bool al, bool addr, u8 regRefHi, u8 regRefLo> void Core_z80::op_ex() {
    if (!addr) lastCycle();
    _sync(1);
	u8 dataL, dataH;

	if (addr) {
        dataL = readByte(reg.w.SP, 3);
        dataH = readByte(reg.w.SP + 1, 4);
		exchange(&dataL, regPTR[regRefLo]);
		exchange(&dataH, regPTR[regRefHi]);
        writeByte(reg.w.SP, dataL, 3);
        _sync(2);
        writeByte(reg.w.SP + 1, dataH, 3, true);
	} else if (!al) {
		exchange(&reg.b.D, &reg.b.H);
		exchange(&reg.b.E, &reg.b.L);
	} else if (regRefLo == _F) {
		exchange(&reg.b.A, &reg_al.b.A);
		exchange(&reg.b.F, &reg_al.b.F);
	} else {
		exchange(&reg.b.C, &reg_al.b.C);
		exchange(&reg.b.B, &reg_al.b.B);
		exchange(&reg.b.L, &reg_al.b.L);
		exchange(&reg.b.H, &reg_al.b.H);
		exchange(&reg.b.E, &reg_al.b.E);
		exchange(&reg.b.D, &reg_al.b.D);
	}
}

template<bool inc, bool _8bit, bool addr, u8 regRef> void Core_z80::op_dec_inc() {
    if (_8bit && !addr) lastCycle();
    _sync(1);
	displacement = 0;

	if (addr) {
		if (regRef == _IX || regRef == _IY) {
			displacement = readPc(3);
            _sync(1);
		}
        _sync(1);
        data8 = readByte(*regPTR16[getReg16(regRef)] + i8(displacement), 3);
        if (regRef == _IX || regRef == _IY) _sync(4);
	}

	if (_8bit) {
		u8 result = doIncDec<inc>(addr ? data8 : *regPTR[regRef]);

        if (addr) writeByte(*regPTR16[getReg16(regRef)] + i8(displacement), result, 3, true);
		else *regPTR[regRef] = result;
	} else {
        _sync(1);
        lastCycle();
        _sync(1);
        *regPTR16[getReg16(regRef)] = *regPTR16[getReg16(regRef)] + (inc ? 1 : -1);
	}
}

template<u8 destRef, u8 srcRef, bool _8bit, bool addr, u16 (Core_z80::*op_l)(u16 src, u16 dest)>
void Core_z80::op_arithmetic() {
    if (srcRef != _DATA && _8bit && !addr) {
        lastCycle();
    }
    _sync(1);

    if (_8bit) {
        if (srcRef == _DATA) data8 = readPc(3, true);
        else if (addr) {
            displacement = 0;
            if (srcRef == _IX || srcRef == _IY) {
                displacement = readPc(3);
                _sync(5);
            }
            data8 = readByte(*regPTR16[getReg16(srcRef)] + i8(displacement), 3, true);
        }

        u8 temp = (this->*op_l)(reg.b.A, srcRef == _DATA || addr ? data8 : *regPTR[srcRef]);
        if (destRef != _NONE ) reg.b.A = temp;
        else setUndocumentedFlags( srcRef == _DATA || addr ? data8 : *regPTR[srcRef] ); //cp special
    } else {
        *regPTR16[getReg16(destRef)] = (this->*op_l)(*regPTR16[getReg16(destRef)], *regPTR16[getReg16(srcRef)]);
        _sync(6);
        lastCycle();
        _sync(1);
    }
}

template<u8 mode> void Core_z80::op_im() {
    lastCycle();
    _sync(1);
	intr.imode = mode;
}

void Core_z80::op_halt() {
    lastCycle();
    _sync(1);
	stop = true;
	while(stop) {
		handle_interrupts();
		if (!stop) break;
		incR();
        _sync(3);
        lastCycle();
        _sync(1);
	}
}

void Core_z80::op_nop() {
    lastCycle();
    _sync(1);
}

void Core_z80::op_ccf() {
    lastCycle();
    _sync(1);
	reg.bit.h = reg.bit.c;
	reg.bit.n = 0;
	setUndocumentedFlags(reg.b.A);
	reg.bit.c = !reg.bit.c;
}

void Core_z80::op_scf() {
    lastCycle();
    _sync(1);
	reg.bit.c = 1;
	reg.bit.h = reg.bit.n = 0;
	setUndocumentedFlags(reg.b.A);
}

void Core_z80::op_neg() {
    lastCycle();
    _sync(1);
	reg.b.A = doSub8(0, reg.b.A);
}

void Core_z80::op_cpl() {
    lastCycle();
    _sync(1);
	reg.b.A = ~reg.b.A;
	reg.bit.h = reg.bit.n = 1;
	setUndocumentedFlags(reg.b.A);
}

void Core_z80::op_daa() {
    lastCycle();
    _sync(1);
	u8 correction = 0;
	u8 temp = reg.b.A;

	if (reg.bit.c || (reg.b.A > 0x99) ) {
		reg.bit.c = 1;
		correction = 6 << 4;
	} else reg.bit.c = 0;

	if (reg.bit.h || ( (reg.b.A & 0x0F) > 0x09) ) correction |= 6;

	if (!reg.bit.n) {
		reg.b.A += correction;
	} else {
		reg.b.A -= correction;
	}

	reg.bit.h = (temp ^ reg.b.A) & 0x10;
	setUndocumentedFlags(reg.b.A);
    setStandardFlags(reg.b.A);
}

template<u8 regRef, bool addr, u8 mode, u8 adjust> void Core_z80::op_bitM() {
    if (!addr) lastCycle();
    _sync(1);
	if (addr) {
        data8 = readByte( *regPTR16[getReg16(regRef)], 4);
	} else {
		data8 = *regPTR[regRef];
	}
	switch ( mode ) {
		case _SET: data8 = doSet(adjust, data8); break;
		case _RESET: data8 = doRes(adjust, data8); break;
		case _RL: data8 = doRL<false, adjust> (data8); break;
		case _RLC: data8 = doRL<true, adjust> (data8); break;
		case _RR: data8 = doRR<false, adjust> (data8); break;
		case _RRC: data8 = doRR<true, adjust> (data8); break;
		case _SLL: data8 = doSL<false> (data8); break;
		case _SLA: data8 = doSL<true> (data8); break;
		case _SRL: data8 = doSR<false> (data8); break;
		case _SRA: data8 = doSR<true> (data8); break;
	}
	if (addr) {
        writeByte(*regPTR16[getReg16(regRef)], data8, 3, true);
	} else {
        *regPTR[regRef] = data8;
	}
}

void Core_z80::op_rld() {
    _sync(1);
    data8 = readByte(reg.w.HL, 3);
	writeByte(reg.w.HL, data8 << 4 | (data8 & 0xF), 4);
	u8 nb = reg.b.A;
	reg.b.A = (data8 >> 4) | (reg.b.A & 0xF0);
    writeByte(reg.w.HL, data8 << 4 | (nb & 0xF), 3, true);
    setStandardFlags(reg.b.A);
	setUndocumentedFlags(reg.b.A);
	reg.bit.n = reg.bit.h = 0;
}

void Core_z80::op_rrd() {
    _sync(1);
    data8 = readByte(reg.w.HL, 3);
	u8 nb = reg.b.A;
	reg.b.A = (data8 & 0xF) | (reg.b.A & 0xF0);
    writeByte(reg.w.HL, nb << 4 | (data8 & 0xF), 4);
    writeByte(reg.w.HL, nb << 4 | (data8 >> 4), 3, true);
    setStandardFlags(reg.b.A);
	setUndocumentedFlags(reg.b.A);
	reg.bit.n = reg.bit.h = 0;
}

template<u8 Ref, bool addr, u8 bit> void Core_z80::op_bit() {
    if (!addr) lastCycle();

    if (Ref != _IX && Ref != _IY) {
        _sync(1);
        displacement = 0;
    }
	if (addr) {
        data16 = *regPTR16[getReg16(Ref)] + i8(displacement);
        data8 = readByte( data16 , 4, true);
	}
	data8 = (addr ? data8 : *regPTR[Ref]) & (1 << bit);
	reg.b.F &= FLAG_C;
	reg.bit.h = 1;
	if ( data8 == 0) {
		reg.bit.pv = reg.bit.z = 1;
	} else {
		if (bit == 7) reg.bit.s = 1;
	}

	if (!addr) {
		setUndocumentedFlags( *regPTR[Ref] );
	} else {
		if (Ref == _HL) setUndocumentedFlags( reg.b.H );
		else setUndocumentedFlags( (data16 >> 8) & 0xFF );
	}
}

template<i8 cond, u8 regRef> void Core_z80::op_jp() {
    if (regRef != _DATA) lastCycle();
    _sync(1);
    if (regRef == _DATA) {
		data16 = readPc(3);
        data16 |= readPc(3, true) << 8;
	}

	if (cond == -1 || getCC(cond)) {
        reg.w.PC = regRef == _DATA ? data16 : *regPTR16[getReg16(regRef)];
	}
}

template<i8 cond> void Core_z80::op_jr() {
    _sync(1);
    bool _branch = cond == -1 || getCC(cond);
    displacement = readPc(3, _branch ? false : true);
    if (_branch) {
		reg.w.PC += i8(displacement);
        _sync(4);
        lastCycle();
        _sync(1);
	}
}

void Core_z80::op_djnz() {
    _sync(2);
    --reg.b.B;
    displacement = readPc(3, reg.b.B ? false : true);
    if (reg.b.B) {
		reg.w.PC += i8(displacement);
        _sync(4);
        lastCycle();
        _sync(1);
	}
}

template<i8 cond> void Core_z80::op_call() {
    _sync(1);
    bool _branch = cond == -1 || getCC(cond);
	data16 = readPc(3);
    data16 |= readPc(3, _branch ? false : true) << 8;
    if (_branch) {
        _sync(1);
        writeStack(reg.w.PC, true);
		reg.w.PC = data16;
	}
}

template<i8 cond> void Core_z80::op_ret() {
    _sync(1);
    bool _res = true;
    if (cond != -1) {
        _res = getCC(cond);
        if (!_res) lastCycle();
        _sync(1);
    }
    if (cond == -1 || _res) {
		reg.w.PC = readStack();
	}
}

void Core_z80::op_reti() {
    _sync(1);
	reg.w.PC = readStack();
	intr.iff1 = intr.iff2;
}

void Core_z80::op_retn() {
    _sync(1);
	reg.w.PC = readStack();
	intr.iff1 = intr.iff2;
}

template<u8 begin> void Core_z80::op_rst() {
    _sync(2);
    writeStack(reg.w.PC, true);
	reg.b.PCh = 0;
	reg.b.PCl = begin;
}

template<u8 regRef> void Core_z80::op_out() {
    _sync(1);
	if (regRef == _DATA) {
		data8 = readPc(3);
        writeIo(data8, reg.b.A, true);
	} else if (regRef == _NONE) {
        writeIo(reg.b.C, 0, true);
	} else {
        writeIo(reg.b.C, *regPTR[regRef], true);
	}
}

template<u8 regRef> void Core_z80::op_in() {
    _sync(1);
	if (regRef == _DATA) {
		data8 = readPc(3);
        reg.b.A = readIo(data8, 4, true);
	} else {
        data8 = readIo(reg.b.C, 4, true);
        setStandardFlags(data8);
		setUndocumentedFlags(data8);
		reg.bit.h = reg.bit.n = 0;
		if(regRef != _F) *regPTR[regRef] = data8;
	}
}

template<bool repeat, bool inc> void Core_z80::op_outx() {
    _sync(2);
    data8 = readByte(reg.w.HL, 3);
    reg.b.B--;
    writeIo(reg.b.C, data8, repeat && reg.b.B ? false : true);
	reg.w.HL += inc ? 1 : -1;

	calcInOutIncDecFlags<inc, false>();

	if (!repeat) return;
	if ( reg.b.B ) {
		reg.w.PC -= 2;
        _sync(4);
        lastCycle();
        _sync(1);
	}
}

template<bool repeat, bool inc> void Core_z80::op_inx() {
    _sync(2);
    data8 = readIo(reg.b.C, 3, false);
    reg.b.B--;
    writeByte(reg.w.HL, data8, 4, repeat && reg.b.B  ? false : true);
    reg.w.HL += inc ? 1 : -1;

	calcInOutIncDecFlags<inc, true>();

	if (!repeat) return;
	if ( reg.b.B ) {
		reg.w.PC -= 2;
        _sync(4);
        lastCycle();
        _sync(1);
	}
}
