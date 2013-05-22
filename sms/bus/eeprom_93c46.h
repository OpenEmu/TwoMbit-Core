#ifndef EEPROM_93C46_H
#define EEPROM_93C46_H

#include "dataTypes.h"
#include "../mem.h"

class Serializer;

class Eeprom_93c46
{
private:
    MemSystem& sram;
    bool enabled;

    enum InitMode { NORMAL, ALL };
    enum { DATA_IN = 0x01, DATA_OUT = 0x08, CLOCK = 0x02, CS = 0x04, DATA_OUT_POS = 3 };
    enum State { START, OPCODE, READING, WRITING };

    void init(InitMode initmode);

    u8 Lines;
    u16 opcode;
    State state;
    bool readOnly;
    u8 position;
    u16 latch;

    u16* getHandle() { return (u16*)sram.handle(); }
    void calc(u8 value);
    void extended();

public:
    bool isEnabled() { return enabled; }
    void setLines(u8 value);
    void control(u8 value);
    void directWrite(u32 addr, u8 value);
    u8 read();
    u8 directRead(u32 addr);
    Eeprom_93c46( MemSystem& _sram);
    void serialize(Serializer& s);
};

#endif // EEPROM_93C46_H
