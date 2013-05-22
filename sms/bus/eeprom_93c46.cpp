//based on meka implementation

#include "eeprom_93c46.h"

Eeprom_93c46::Eeprom_93c46( MemSystem& _sram)
    : sram(_sram)
{
    enabled = false;
}

void Eeprom_93c46::calc(u8 value) {
    u16* _mem = getHandle();
    u8 data = value & DATA_IN;
    Lines = (Lines & ~0x07) | (value & 0x07);
    switch (state) {
        case START:
            if (data) {
                state = OPCODE;
                opcode = 0x0000;
                position = 0;
            } return;
        case OPCODE:
            opcode = (opcode << 1) | data;
            if (++position == 8) {
                switch (opcode & 0xC0) {
                    case 0x00: extended(); return;
                    case 0x40:
                        position = 0;
                        latch = 0x0000;
                        state = WRITING;
                        return;
                    case 0x80:
                        position = 0;
                        state = READING;
                        Lines &= ~DATA_OUT; // Dummy Zero
                        return;
                    case 0xC0:
                        if (readOnly) return;
                        _mem[opcode & 0x3F] = 0xFFFF;
                        Lines |= DATA_OUT; // Ready
                        state = START;
                        return;
                    }
                } return;
        case READING:
            if (_mem [opcode & 0x3F] & (0x8000 >> position))
                Lines |=  DATA_OUT; // Bit 1
            else Lines &= ~DATA_OUT; // Bit 0
            if (++position == 16) {
                position = 0;
                opcode = 0x80 | ((opcode + 1) & 0x3F);
            }
            return;
        case WRITING:
            latch = (latch << 1) | data;
            if (++position == 16) {
                if (!readOnly) {
                    if ((opcode & 0x40) == 0x40) {
                        _mem [opcode & 0x3F] = latch;
                    } else {
                        for (int i = 0; i < 64; i++) {
                            _mem [i] = latch;
                        }
                    }
                }
                Lines |= DATA_OUT;
                state = START;
            }
            return;
        }
}

void Eeprom_93c46::extended() {
    switch (opcode & 0x30) {
        case 0x00:
            readOnly = true;
            state = START;
            return;
        case 0x10:
            position = 0;
            latch = 0x0000;
            state = WRITING;
            return;
        case 0x20:
            if (!readOnly) sram.init(0xff);
            Lines |= DATA_OUT; // Ready
            state = START;
            return;
        case 0x30:
            readOnly = false;
            state = START;
            return;
    }
}

void Eeprom_93c46::setLines(u8 value) {
    if (!(value & CS)) {
        if (Lines & CS) init ( NORMAL );
        Lines = (Lines & ~0x07) | (value & 0x07) | DATA_OUT;
        return;
    } else if ((value & CLOCK) && !(Lines & CLOCK)) {
       calc(value);
       return;
    }
    Lines = (Lines & ~0x07) | (value & 0x07);
}

void Eeprom_93c46::control(u8 value) {
    if (value & 0x80) {
        init( ALL );
        return;
    }
    enabled = value & 0x08;
}

void Eeprom_93c46::directWrite(u32 addr, u8 value) {
    sram.write(addr, value);
}

u8 Eeprom_93c46::read() {
    return (Lines & CS) | ( (Lines & DATA_OUT) >> DATA_OUT_POS ) | CLOCK;
}

u8 Eeprom_93c46::directRead(u32 addr) {
    return sram.read(addr);
}

void Eeprom_93c46::init(InitMode initmode) {
    if (initmode == ALL) {
        enabled = false;
        Lines = 0x00 | DATA_OUT;
        readOnly = true;
    }
    state = START;
    opcode = 0x0000;
    position = 0;
}
