
#ifndef bus_h
#define bus_h

#include "../mem.h"
#include "../cart/cart.h"

class _System;
class Cpu;
class Bus;
class Vdp;
class Cheats;
class Eeprom_93c46;
class Serializer;

class Bus : public Mmio
{
public:
    static u8 flipTable[256];
	u8 read_from(u16 addr);
	void write_to(u16 addr, u8 value);
	u8 mmio_read(u8 pos);
	void mmio_write(u8 pos, u8 value);
    void serialize(Serializer& s);

	void power();
    void recover();
    void setTerebi(bool state) { useTerebi = state; }

	bool biosDetected;
	bool romDetected;
    Cheats* cheats;

	struct MemMmio {
	    _System* sys;
		u8 route(u8 addr);
		u8 ggRoute(u8 addr);
		u8 sgRoute(u8 addr);
		u8 smsRoute(u8 addr);
		u8 read(u8 addr);
		void write(u8 addr, u8 value);
		void map(u8 pos, Mmio* access);
        Mmio* _mmio[19];
		MemMmio(_System* sys) : sys(sys) {}
	} mmio;

	bool isWorkRamDisabled() {
		return (port_3e & 0x10) != 0;
	}
	//Expansion Port and Card are not present
	//all games are handled as cartridge games
	bool isCartridgeEnabled() {
		return (port_3e & 0x40) == 0;
	}
	bool isBiosEnabled() {
		return (port_3e & 0x08) == 0;
	}
	bool isIoDisabled() {
		return (port_3e & 0x04) != 0;
	}

    Bus(_System* sys, Cart& cart, Cpu& cpu, Vdp& vdp, MemStatic& _wram, MemStatic& _wramSG, MemSystem& _rom, MemSystem& _sram, MemSystem& _bios, MemSystem& _cartWram);
	~Bus() {}

private:
    bool useEeprom;
    bool useTerebi;
    bool useByteFlip;
	enum Slot { S_Cartridge = 0, S_Bios = 1 };


	void mapUpdated() {
		page0update = page1update = page2update = false;
	}

    bool cartActive() { return isCartridgeEnabled() && romDetected; }
    bool biosActive() { return biosDetected && isBiosEnabled(); }

	Cart& cart;
	Cpu& cpu;
	Vdp& vdp;
	_System* sys;
    Eeprom_93c46* eeprom;

	u8 port_3e;

    MemStatic& wram;
    MemStatic& wramSG;
    MemSystem& rom;
    MemSystem& sram;
    MemSystem& bios;
    MemSystem& cartWram;

	bool page0update;
	bool page1update;
	bool page2update;

	struct Slots {
		MemMap* access;
        u8 ramSelector;
		u8 page0bank;
		u8 page1bank;
		u8 page2bank;
		u8 page3bank;
		u8 page4bank;
		Cart::Mappers type;
        bool page0flip;
        bool page1flip;
	} slot[2];

	struct Mapper {
		MemMap* access1;
		MemMap* access2;
		u32 offset1;
		u32 offset2;
    } mapper[256];

	struct MemUnmapped : MemMap {
		u8 read(u32 addr);
		void write(u32 addr, u8 value) {}
		MemUnmapped(MemMap::Type _type) { type = _type; }
	} unmapped;

    void map( u8, u8, MemMap&, u32 = 0);
    void unmap( u8 bank_lo, u8 bank_hi, u8 layer );
	void mapWram();
	void mapSram(u8 bankLo, u8 bankHi, Slot _slot, unsigned = 0);
	void mapSystem(Slot _slot, u8 data = 0);
	u8 calcSwift(Slot _slot, u8 bank);
	u32 mirror(u32 index, u32 size);

	void mapRamSelectorWrite(Slot _slot, u8 value);
    void mapSlot2WriteC(u8 value);
	void mapSlot2Write(Slot _slot, u8 value);
	void mapSlot1Write(Slot _slot, u8 value);
	void mapSlot0Write(Slot _slot, u8 value);
	void mapKorean8kWrite(Slot _slot, u32 addr, u8 value);

    void writeMemoryControl(u32 addr, u8 value);
    void writeMemoryControlBios(u32 addr, u8 value);

    void writeEeprom(u16 addr, u8 value, bool& use);
    u8 readEeprom(u16 addr, bool& use);
    u8 readTerebi(u16 addr, bool& use);
    void setRomUsage();
    void initWram();
    void initPagging();
    void initPort3e();
    void remapAll();
    void initCustom();

    void updateBiosGG();
    void updateBios();
    void updateCart();
    void remapAfterRecover();
    void sramInWramArea(Slot _slot);
    void initCartMap();
    void buildFlipTable();
    u8 flipByte(u8 pos, u8 _byte);
};

#endif
