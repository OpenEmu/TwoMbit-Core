
#include "input.h"
#include "serializer.h"

void _Input::serialize(Serializer& s) {

    s.integer(terebi.x);
    s.integer(terebi.y);
    s.integer(terebi.useX);

    for(unsigned i = 0; i < 2; i++) {
        s.integer(lightphaser[i].x);
        s.integer(lightphaser[i].y);
        s.integer(lightphaser[i].offscreen);

        s.integer(paddle[i].flip);
        s.integer(paddle[i].pos);
        s.integer(paddle[i].counter);

        s.integer(sportspad[i].posX);
        s.integer(sportspad[i].posY);
        s.integer(sportspad[i].strobeNibble);
        s.integer(sportspad[i].clock);
        s.integer(sportspad[i].latch);
    }
}
