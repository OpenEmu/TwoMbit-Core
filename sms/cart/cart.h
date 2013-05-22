
#ifndef CART_H
#define CART_H

#include "dataTypes.h"
#include <stdio.h>
#include <string>

using namespace std;

class _System;
class Bus;

class Cart
{
public:
    enum Mappers { BIOS, STANDARD, CODEMASTERS, KOREAN, KOREAN_8K, KOREAN_8K_NEMESIS, NO_BANK_SWITCHING, CUSTOM_4_PAK, CUSTOM_JANGGUN, NONE };
    enum Region { USA = 0, JAPAN = 1, EUROPE = 2 };

private:
    Bus& bus;
    _System* sys;

    u32 romSize;
    u32 sramSize;
    u32 cartWramSize;
    u8* rom;
    u8* sram;
    u8* cartWram;

    Mappers mapper;
    Region region;
    bool ggInSmsMode;
    bool cartDontExpectPort3e;

    bool loaded;
    string romHash;

    bool copyHeaderChecked;
    bool sgUseRamExtension;
    unsigned cartWramInfoPos;

    struct BiosInfo {
        string hash;
        Region region;
    } biosInfo[10];

    struct BackupRamInfo {
        string hash;
        unsigned sramSize;
    } backupRamInfo[24];

    struct SmsModeInfo {
        string hash;
    } smsModeInfo[30];

    struct CartWramInfo {
       string hash;
       unsigned size;
       u8 start;
       u8 end;
    } cartWramInfo[4];

    struct SgWram8K {
       string hash;
    } sgWram8K[1];

    struct GlassesInfo {
        string hash;
    } glassesInfo[12];

    struct PalRegions {
        string hash;
    } palRegions[46];

    struct MapperHint {
        Mappers mapper;
        unsigned hint;
        bool operator<(const MapperHint& mh) const {
            return (hint > mh.hint);
        }
    } mapperHint[5];

    struct WramPattern {
        string hash;
        u8 value;
    } wramPattern[2];

    struct CustomMapper {
        string hash;
        Mappers mapper;
    } customMapper[20];

    struct DisablePort3e { //korean master system?
        string hash;
    } disablePort3e[5];

    unsigned mapperHintSize;
    unsigned backupRamInfoSize;
    unsigned biosInfoSize;
    unsigned glassesInfoSize;
    unsigned palRegionsSize;
    unsigned smsModeInfoSize;
    unsigned cartWramInfoSize;
    unsigned wramPatternSize;
    unsigned customMapperSize;
    unsigned disablePort3eSize;
    unsigned sgWram8KSize;

    bool detectHeader();
    void resetHints();
    void hintMapper(Mappers _mapper, unsigned add = 1);
    unsigned getMapperHint(Mappers _mapper);
    bool isAnyMapperHinted(unsigned limit);
    void detectMapper();
    void setMapper();
    void setRegion();
    void setSram();
    void calcHash();
    unsigned getCustomSramSize();
    bool checkForSmsMode();
    bool checkForPalRegion();
    bool isBios();
    bool is3dGlasses();
    void removeNonRomHeader();
    void toLower(string& str);
    bool useCartWram();
    bool isPort3eDisable();
    Mappers getCustomMapper();

public:
    void load(u8* _rom, u32 _romSize, bool _ggInSmsMode = false);

    unsigned getSramSize();
    void getSgWramSize();
    unsigned getCartWramSize();
    unsigned getWramPattern();
    u8* getSram();

    void unload();
    bool isLoaded() { return loaded; }
    Mappers getMapper() {
        return mapper;
    }
    bool isGgInSmsMode() {
        return ggInSmsMode;
    }
    bool isPort3eNotExpected() {
        return cartDontExpectPort3e;
    }
    bool sgUse8kRam() {
        return sgUseRamExtension;
    }
    unsigned getCartWramMapStart() {
        return cartWramInfo[cartWramInfoPos].start;
    }
    unsigned getCartWramMapEnd() {
        return cartWramInfo[cartWramInfoPos].end;
    }

	Cart(_System* sys, Bus& bus);
	~Cart();
};

#endif
