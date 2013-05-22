#include "eeprom_93c46.h"
#include "serializer.h"

void Eeprom_93c46::serialize(Serializer& s) {
    s.integer(enabled);
    s.integer(Lines);
    s.integer(opcode);
    unsigned _state = state;
    s.integer(_state);
    state = (State)_state;
    s.integer(readOnly);
    s.integer(position);
    s.integer(latch);
}

