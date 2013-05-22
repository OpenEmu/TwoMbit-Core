#ifndef CHEATS_H
#define CHEATS_H

#define MAX_CHEATS 20

#include "dataTypes.h"
//#include <string/_string.h>

class Cheats
{
    enum Mode { ACTION_REPLAY, GAME_GENIE };
    static Cheats* instance;

    struct Props {
        unsigned addr;
        u8 replaceData;
        u8 originalData;
        Mode mode;
    } list[MAX_CHEATS];

    unsigned inUse;
    bool cheatsActivated;

    //bool decode(string input, Props& props);

public:
    bool set(const char* code );
    void resetList();
    u8 read(unsigned addr, u8 originalData);

    static Cheats* getInstance() {
        if (!instance) {
            instance = new Cheats();
        }
        return instance;
    }

    Cheats();
};

#endif // CHEATS_H
