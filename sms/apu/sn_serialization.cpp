
#include "sn76489.h"
#include "serializer.h"

void SN76489::serialize(Serializer& s) {
    s.array(registers, 8);
    s.array(phase, 4);
    s.array(counter, 4);
    s.array(usedAmp, 4);
    s.integer(latched);
    s.integer(NoiseShiftRegister);
    s.integer(NoiseFreq);
    s.integer(ggDirection);
    s.integer(isGGMode);

    s.integer(chipClock);
}
