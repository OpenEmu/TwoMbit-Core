#include "cheats.h"
#include <algorithm>

Cheats* Cheats::instance = 0;

Cheats::Cheats()
{
    inUse = 0;
    cheatsActivated = false;
}

bool Cheats::set( const char* code ) {
    if (inUse >= MAX_CHEATS) {
        return false;
    }
    Props props;
    /*
    if ( !decode( code, props) ) {
        return false;
    }*/

    list[inUse++] = props;
    cheatsActivated = true;
    return true;
}

void Cheats::resetList() {
    inUse = 0;
    cheatsActivated = false;
}

u8 Cheats::read(unsigned addr, u8 originalData) {
    if (!cheatsActivated) {
        return originalData;
    }
    for(unsigned i = 0; i < inUse; i++) {
        if (list[i].addr == addr) {
            if (list[i].mode == GAME_GENIE) {
                if (list[i].originalData == originalData) {
                    return list[i].replaceData;
                }
                return originalData;
            }
            return list[i].replaceData;
        }
    }
    return originalData;
}

/*bool Cheats::decode(string input, Props& props) {
    string str;
    __String::removeChars(input, ":- ");
    transform(input.begin(), input.end(), input.begin(), ::tolower);

    try {
        if (input.length() == 9) { //game genie
            props.mode = GAME_GENIE;
            str = input.substr(0, 2);
            props.replaceData = __String::hexToInt<unsigned>(str);
            str = input.substr(5, 1);
            u8 F = __String::hexToInt<unsigned>(str);
            str = input.substr(2, 3);
            unsigned CDE = __String::hexToInt<unsigned>(str);
            props.addr = ((F ^ 0xf) << 12) | CDE;
            str = input.substr(6, 1);
            unsigned GI = __String::hexToInt<unsigned>(str) << 4;
            str = input.substr(8, 1);
            GI |= __String::hexToInt<unsigned>(str);
            props.originalData = (((GI >> 2) | (GI << 6)) & 0xff ) ^ 0xba;
            return true;
        }

        if ( (input.length() != 8) && (input.length() != 6)) {
            return false;
        }
        props.mode = ACTION_REPLAY;
        if (input.length() == 8) {
            str = input.substr(0, 2);
            u8 _data = __String::hexToInt<unsigned>(str);
            if (_data > 0) {
                props.originalData = _data;
                props.mode = GAME_GENIE;
            }
            input = input.substr(2);
        }

        str = input.substr(4, 2);
        props.replaceData = __String::hexToInt<unsigned>(str);
        str = input.substr(0, 4);
        props.addr = __String::hexToInt<unsigned>(str);
        return true;
    } catch (...) {
        return false;
    }
}*/


