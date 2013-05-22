
void writeIo(u8 addr, u8 value, bool last = false) {
    if (!last) _sync(4);
    else {
        _sync(3);
        lastCycle();
        _sync(1);
    }
	ioWrite(addr, value);
}

u8 readIo(u8 addr, u8 timing, bool last = false) {
	_sync(timing - 1); //set address on bus
	mdr = ioRead(addr);
    if (last) lastCycle();
	_sync(1);
	return mdr;
}

void writeByte(u16 addr, u8 value, u8 timing, bool last = false) {
    if (!last) _sync(timing);
    else {
        _sync(timing-1);
        lastCycle();
        _sync(1);
    }
	memWrite(addr, value);
}

void writeStack(u16 value, bool last = false) {
    writeByte(--reg.w.SP, value >> 8, 3);
    writeByte(--reg.w.SP, value & 0xFF, 3, last);
}

u8 readByte(u16 addr, u8 timing, bool last = false) {
    _sync( timing - 1 );
	mdr = memRead(addr);
    if (last) lastCycle();
    _sync( 1 );
	return mdr;
}

u8 readPc(u8 timing, bool last = false) {
    u8 data = readByte(reg.w.PC, timing, last);
	reg.w.PC++;
	return data;
}

u16 readStack() {
	u16 data;
    data = readByte(reg.w.SP++, 3);
    data |= readByte(reg.w.SP++, 3, true) << 8;
	return data;
}

