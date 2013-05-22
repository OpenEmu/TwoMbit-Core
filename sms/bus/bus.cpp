
#include "bus.h"
#include "../system/system.h"
#include "../cpu/cpu.h"
#include "../vdp/vdp.h"
#include "../cheats/cheats.h"
#include "eeprom_93c46.h"

u8 Bus::flipTable[256];

Bus::Bus(_System* sys, Cart& cart, Cpu& cpu, Vdp& vdp, MemStatic& _wram, MemStatic& _wramSG, MemSystem& _rom, MemSystem& _sram, MemSystem& _bios, MemSystem& _cartWram)
:
sys(sys),
cart(cart),
cpu(cpu),
vdp(vdp),
cheats(cheats),
wram(_wram),
wramSG(_wramSG),
rom(_rom),
sram(_sram),
bios(_bios),
cartWram(_cartWram),
mmio(sys),
unmapped(MemMap::UNMAPPED) {
	mmio.map(0, this);
	slot[S_Cartridge].access = &rom;
    slot[S_Bios].access = &bios;
	slot[S_Bios].type = Cart::BIOS;
    eeprom = new Eeprom_93c46(sram);
    useTerebi = false;
    cheats = Cheats::getInstance();
    buildFlipTable();
}

u8 Bus::mmio_read(u8 pos) {
	return cpu.openBus();
}

void Bus::mmio_write(u8 pos, u8 value) {
    if (cart.isPort3eNotExpected()) {
        return;
    }
	bool biosUpdate = false;
	bool cartUpdate = false;
	bool wramUpdate = false;

	if ( ((port_3e & 0x08) == 0) != ((value & 0x08) == 0) ) {
	    biosUpdate = true;
	}
	if ( ((port_3e & 0x40) == 0) != ((value & 0x40) == 0) ) {
		cartUpdate = true;
	}

	if ( ((port_3e & 0x10) == 0) != ((value & 0x10) == 0) ) {
	    wramUpdate = true;
    }

	if ( ( ((port_3e & 0x20) == 0) != ((value & 0x20) == 0) ) //card
		|| ( ((port_3e & 0x80) == 0) != ((value & 0x80) == 0) ) ) { //expansion
	}

    port_3e = value;

	if (wramUpdate) {
        mapWram();
	}

	if (biosUpdate && (sys->getUnit() == _System::GG)) {
        updateBiosGG();
        return;
	}

	if (biosUpdate && sys->getUnit() == _System::SMS) {
        updateBios();
	}
    if (cartUpdate && sys->getUnit() == _System::SMS) {
        updateCart();
	}
}

void Bus::updateBiosGG() {
    unmap(0x00, 0x03, 0);
    unmap(0x00, 0x03, 1);
    if ( ( (port_3e & 0x08) == 0 ) && biosDetected ) {
        map(0x00, 0x03, *slot[S_Bios].access);
    } else if (romDetected) {
        map(0x00, 0x03, *slot[S_Cartridge].access);
    }
}

void Bus::updateBios() {
    if ( isBiosEnabled() && biosDetected ) {
        page0update = page1update = page2update = true;
        mapSystem(S_Bios);
    } else {
        unmap( 0x00, 0xbf, 1 );
    }
}

void Bus::updateCart() {
    if ( isCartridgeEnabled() && romDetected) {
        page0update = page1update = page2update = true;
        mapSystem(S_Cartridge);
    } else {
        unmap( 0x00, 0xbf, 0 );
    }
}

void Bus::remapAfterRecover() {
    if (sys->getUnit() == _System::SMS) {
        updateBios();
        updateCart();
        sramInWramArea(S_Cartridge);
    } else if (sys->getUnit() == _System::GG) {
        if (romDetected) {
            page0update = page1update = page2update = true;
            mapSystem(S_Cartridge);
        }
        updateBiosGG();
        sramInWramArea(S_Cartridge);
    } else {
        page0update = page1update = page2update = true;
        mapSystem(S_Cartridge);
    }
    initCustom();
}

void Bus::mapSram(u8 bankLo, u8 bankHi, Slot _slot, unsigned offset) {
	if (_slot == S_Cartridge && sram.size() != 0) {
        map(bankLo, bankHi, sram, offset );
	} else {
        unmap( bankLo, bankHi, _slot == S_Cartridge ? 0 : 1 );
	}
}

void Bus::mapWram() {
    if (isWorkRamDisabled()) {
        unmap( 0xc0, 0xff, 1 );
        return;
    }
    MemMap& access = sys->getUnit() == _System::SG1000 && !cart.sgUse8kRam() ? wramSG : wram;
    map(0xc0, 0xdf, access); //wram
    map(0xe0, 0xff, access); //mirror wram
}

void Bus::mapSystem(Slot _slot, u8 data) {
	switch ( slot[_slot].type ) {
		case Cart::STANDARD:
		case Cart::BIOS:
			if (page0update) {
                map(0x00, 0x03, *slot[_slot].access); //first k of rom
                map(0x04, 0x3f, *slot[_slot].access, calcSwift(_slot, slot[_slot].page0bank) * 0x4000 + 0x400); //15k slot 0
			}
            if (page1update) map(0x40, 0x7F, *slot[_slot].access, calcSwift(_slot, slot[_slot].page1bank) * 0x4000); //16k slot 1
			if (page2update) {
				if (slot[_slot].ramSelector & 0x8) { //map sram
                    mapSram(0x80, 0xbf, _slot, slot[_slot].ramSelector & 0x4 ? 16 * 1024 : 0);
				} else {
                    map(0x80, 0xbf, *slot[_slot].access, calcSwift(_slot, slot[_slot].page2bank) * 0x4000); //16k slot 2
				}
			}
			break;
        case Cart::CUSTOM_4_PAK:
            if (page0update) map(0x00, 0x3F, *slot[_slot].access, slot[_slot].page0bank * 0x4000); //16k slot 0
            if (page1update) map(0x40, 0x7F, *slot[_slot].access, slot[_slot].page1bank * 0x4000); //16k slot 1
            if (page2update) map(0x80, 0xBF, *slot[_slot].access, slot[_slot].page2bank * 0x4000); //16k slot 2
            break;
        case Cart::CODEMASTERS:
			if (page0update) {
                map(0x00, 0x3F, *slot[_slot].access, slot[_slot].page0bank * 0x4000); //16k slot 0
            } if (page1update) {
                map(0x40, 0x7F, *slot[_slot].access, slot[_slot].page1bank * 0x4000); //16k slot 1
                if ((data & 0x80) && (cartWram.size() != 0)) map(0xa0, 0xbf, cartWram);
            } if (page2update) {
                map(0x80, 0xBF, *slot[_slot].access, slot[_slot].page2bank * 0x4000); //16k slot 2
                if ((data & 0x80) && (cartWram.size() != 0)) map(0xa0, 0xbf, cartWram);
			}
			break;
        case Cart::NO_BANK_SWITCHING:
            map(0x00, 0x3F, *slot[_slot].access); //16k slot 0
            map(0x40, 0x7F, *slot[_slot].access, 0x4000); //16k slot 1
            map(0x80, 0xbf, *slot[_slot].access, 2 * 0x4000); //16k slot 2
            if (cartWram.size() != 0) {//the castle, othello, kings valley have additional cart wram
                map(cart.getCartWramMapStart(), cart.getCartWramMapEnd(), cartWram);
            }
            break;
		case Cart::KOREAN:
            if (page0update) map(0x00, 0x3F, *slot[_slot].access); //16k slot 0
            if (page1update) map(0x40, 0x7F, *slot[_slot].access, 0x4000); //16k slot 1
            if (page2update) map(0x80, 0xBF, *slot[_slot].access, slot[_slot].page2bank * 0x4000); //16k slot 2
			break;
		case Cart::KOREAN_8K:
        case Cart::KOREAN_8K_NEMESIS:
        case Cart::CUSTOM_JANGGUN:
            if (page0update) map(0x00, 0x3F, *slot[_slot].access, slot[_slot].page0bank * 0x4000); //16k slot 0
			if (page1update) {
                map(0x40, 0x5F, *slot[_slot].access, slot[_slot].page1bank * 0x2000); //8k slot 1
                map(0x60, 0x7F, *slot[_slot].access, slot[_slot].page2bank * 0x2000); //8k slot 2
			}
			if (page2update) {
                map(0x80, 0x9F, *slot[_slot].access, slot[_slot].page3bank * 0x2000); //8k slot 3
                map(0xA0, 0xBF, *slot[_slot].access, slot[_slot].page4bank * 0x2000); //8k slot 4
			}
			break;
	}
	mapUpdated();
}

u8 Bus::calcSwift(Slot _slot, u8 bank) {
    return bank;
	switch(slot[_slot].ramSelector & 0x3) {
		case 0: return bank;
		case 1: return (bank + 0x18) & 0x1f;
		case 2: return (bank + 0x10) & 0x1f;
		case 3: return (bank + 0x08) & 0x1f;
	}
}

void Bus::mapRamSelectorWrite(Slot _slot, u8 value) {

	if ((value & 0x03) != (slot[_slot].ramSelector & 0x03)) {
		page0update = page1update = page2update = true;
	}
	else if ((value & 0x04) != (slot[_slot].ramSelector & 0x04)) page2update = true;
	else if ((value & 0x08) != (slot[_slot].ramSelector & 0x08)) page2update = true;

	if ((value & 0x10) != (slot[_slot].ramSelector & 0x10)) {
        slot[_slot].ramSelector = value;
        sramInWramArea(_slot);
	}

	slot[_slot].ramSelector = value;
	mapSystem(_slot);
}

void Bus::sramInWramArea(Slot _slot) {
    if (_slot == S_Cartridge) {
        if (slot[_slot].ramSelector & 0x10) {
            mapSram(0xc0, 0xff, _slot);
        } else {
            unmap( 0xc0, 0xff, 0 );
        }
    }
}

void Bus::mapSlot2WriteC(u8 value) {
    slot[S_Cartridge].page2bank = (slot[S_Cartridge].page0bank & 0x30) + value;
    page2update = true;
    mapSystem(S_Cartridge, value);
}

void Bus::mapSlot2Write(Slot _slot, u8 value) {
    slot[_slot].page2bank = value;
	page2update = true;
	mapSystem(_slot, value);
}

void Bus::mapSlot1Write(Slot _slot, u8 value) {
    slot[_slot].page1bank = value;
	page1update = true;
	mapSystem(_slot, value);
}

void Bus::mapSlot0Write(Slot _slot, u8 value) {
    slot[_slot].page0bank = value;
	page0update = true;
	mapSystem(_slot, value);
}

void Bus::mapKorean8kWrite(Slot _slot, u32 addr, u8 value) {
    switch(addr) {
		case 0x0000:
        case 0x8000:
			slot[_slot].page3bank = value;
			page2update = true;
			break;
		case 0x0001:
        case 0xa000:
			slot[_slot].page4bank = value;
			page2update = true;
			break;
		case 0x0002:
        case 0x4000:
			slot[_slot].page1bank = value;
			page1update = true;
			break;
        case 0x0003:
        case 0x6000:
			slot[_slot].page2bank = value;
			page1update = true;
			break;
	}
	mapSystem(_slot);
}

void Bus::write_to(u16 addr, u8 value) {
    Mapper& m = mapper[addr>>8];
    m.access1->write(m.offset1 | (addr & 0xff), value);
    m.access2->write(m.offset2 | (addr & 0xff), value);
    writeMemoryControl(addr, value);
    if (biosActive()) writeMemoryControlBios(addr, value);
    if (useTerebi) {
        if (addr == 0x6000) {
            cpu.getInput().setTerebiDir(value);
        }
    }
}

u8 Bus::readEeprom(u16 addr, bool& use) {
    use = false;
    if (!eeprom->isEnabled()) return 0;
    if (addr == 0x8000) {
        use = true;
        return eeprom->read();
    } else if (addr >= 0x8008 && addr < 0x8088) {
        use = true;
        return eeprom->directRead(addr - 0x8008);
    }
    return 0;
}

void Bus::writeEeprom(u16 addr, u8 value, bool& use) {
    use = false;
    if (addr == 0x8000) {
        eeprom->setLines(value);
        use = true;
    } else if (addr >= 0x8008 && addr < 0x8088) {
        eeprom->directWrite(addr - 0x8008, value);
        use = true;
    } else if (addr == 0xfffc) {
        eeprom->control(value);
        use = true;
    }
}

u8 Bus::read_from(u16 addr) {
    if (useEeprom || useTerebi) {
        bool use;
        u8 _res;
        if (useEeprom) {
            _res = readEeprom(addr, use);
            if (use) return _res;
        } else if (useTerebi) {
            _res = readTerebi(addr, use);
            if (use) return _res;
        }
    }

    Mapper& m = mapper[addr>>8];
	u8 fetch1 = 0xff, fetch2 = 0xff;
	if (m.access1->type != MemMap::UNMAPPED) {
        fetch1 = m.access1->read(m.offset1 | (addr & 0xff));
        if (useByteFlip) {
            fetch1 = flipByte(addr >> 13, fetch1);
        }
	}
    if (m.access2->type != MemMap::UNMAPPED) {
        fetch2 = m.access2->read(m.offset2 | (addr & 0xff));
	}
    return cheats->read(addr, fetch1 & fetch2);
    //return fetch1 & fetch2;
}

u8 Bus::flipByte(u8 pos, u8 _byte) {
    switch(pos & 7) {
        case 2:
        case 3:
            if (slot[S_Cartridge].page0flip) {
                _byte = flipTable[_byte];
            }
            break;
        case 4:
        case 5:
            if (slot[S_Cartridge].page1flip) {
                _byte = flipTable[_byte];
            }
            break;
    }
    return _byte;
}

u8 Bus::readTerebi(u16 addr, bool& use) {
    use = false;
    if (addr == 0x8000) {
        use = true;
        return cpu.getInput().getTerebiState();
    } else if (addr == 0xa000) {
        use = true;
        return cpu.getInput().getTerebiPos();
    }
}

void Bus::unmap( u8 bank_lo, u8 bank_hi, u8 layer ) {
    for(u16 bank = bank_lo; bank <= bank_hi; bank++) {
        switch (layer) {
            case 0: mapper[bank].access1 = &unmapped; mapper[bank].offset1 = bank; break;
            case 1: mapper[bank].access2 = &unmapped; mapper[bank].offset2 = bank; break;
        }
    }
}

void Bus::map(	u8 bank_lo, u8 bank_hi, MemMap& access, u32 offset) {
	u32 index = 0;
    for(u16 bank = bank_lo; bank <= bank_hi; bank++) {
        switch ( access.type ) {
            case MemMap::WRAM:
            case MemMap::WRAMSG:
            case MemMap::BIOS:
                mapper[bank].access2 = &access;
                mapper[bank].offset2 = mirror(offset + index, access.size());
                break;
            case MemMap::SRAM:
            case MemMap::ROM:
            case MemMap::CARTWRAM:
                if (access.size()) {
                    mapper[bank].access1 = &access;
                    mapper[bank].offset1 = mirror(offset + index, access.size());
                }
                break;
        }
        index += 256;
    }
}

u32 Bus::mirror(u32 index, u32 size) {
	if (index < size) return index;
    //return size % index;

    u32 mask = 1 << 23, part=0;

    for(;;) {
        if(index < size) return part + index;
        while (index < mask) mask >>= 1;
        index -= mask;
        if (size > mask) {
            size -= mask;
            part += mask;
        }
    }
    return 0;
}

void Bus::setRomUsage() {
    biosDetected = bios.size() == 0 ? false : true;
    romDetected = rom.size() == 0 ? false : true;
    slot[S_Cartridge].type = cart.getMapper();
    useEeprom = sram.size() == 128;
    useByteFlip = slot[S_Cartridge].type == Cart::CUSTOM_JANGGUN;

    if (cart.getMapper() == Cart::BIOS) { //only a bios is loaded, maybe with game built in
        biosDetected = true;
        romDetected = false;
        bios.map(rom.handle(), rom.size());
    }
    unmap( 0, 0xff, 0 );
    unmap( 0, 0xff, 1 );
}

void Bus::initPagging() {
    slot[S_Cartridge].page0flip = false;
    slot[S_Cartridge].page1flip = false;
    slot[S_Cartridge].ramSelector = slot[S_Bios].ramSelector = 0x00;
    slot[S_Cartridge].page0bank = slot[S_Bios].page0bank = 0;
    slot[S_Cartridge].page1bank = slot[S_Bios].page1bank = 1;
    slot[S_Cartridge].page2bank = slot[S_Cartridge].type == Cart::STANDARD || slot[S_Cartridge].type == Cart::NO_BANK_SWITCHING ? 2 : 0;
    slot[S_Bios].page2bank = 2;
    //for korean 8k mapper
    slot[S_Cartridge].page3bank = 0;
    slot[S_Cartridge].page4bank = 0;
    if (slot[S_Cartridge].type == Cart::KOREAN_8K || slot[S_Cartridge].type == Cart::KOREAN_8K_NEMESIS) {
        slot[S_Cartridge].page1bank = 0;
        slot[S_Cartridge].page2bank = 0;
    }
}

void Bus::initWram() {
    wram.init( cart.getWramPattern() );
    wramSG.init(0);
}

void Bus::initPort3e() {
    if (sys->getUnit() == _System::GG) {
        port_3e = biosDetected ? 0x3 : 0xb;
    } else {
        port_3e = biosDetected ? 0xe3 : 0xab;
    }
}

void Bus::initCartMap() {
    page0update = page1update = page2update = true;
    if (sys->getUnit() != _System::GG) {
        if (biosDetected) mapSystem(S_Bios);
        else mapSystem(S_Cartridge);
    } else {
        if (romDetected) {
            mapSystem(S_Cartridge);
            if (biosDetected) {
                unmap(0x00, 0x03, 0);
                map(0x00, 0x03, *slot[S_Bios].access);
            }
        } else {
            map(0x00, 0x03, *slot[S_Bios].access);
        }
    }
}

void Bus::power() {
    setRomUsage();
    initWram();
    initPagging();
    initPort3e();
    mapWram();
    initCartMap();
    initCustom();
}

void Bus::initCustom() {
    if (slot[S_Cartridge].type == Cart::KOREAN_8K_NEMESIS) {
        map(0x00, 0x1F, *slot[S_Cartridge].access, 0x1e000);
    }
}

void Bus::recover() {
    setRomUsage();
    mapWram();
    remapAfterRecover();
}

void Bus::writeMemoryControl(u32 addr, u8 value) {

    switch ( slot[S_Cartridge].type ) {
        case Cart::STANDARD:
            if (useEeprom) {
                bool use;
                writeEeprom(addr, value, use);
                if (use) return;
            }
            if (addr == 0xfffc) { if (cartActive()) mapRamSelectorWrite(S_Cartridge, value); }
            else if (addr == 0xfffd) { if (cartActive()) mapSlot0Write(S_Cartridge, value); }
            else if (addr == 0xfffe) { if (cartActive()) mapSlot1Write(S_Cartridge, value); }
            else if (addr == 0xffff)  { if (cartActive()) mapSlot2Write(S_Cartridge, value); }
            return;
        case Cart::CUSTOM_4_PAK:
            if (addr == 0x3ffe) { if (cartActive()) mapSlot0Write(S_Cartridge, value); }
            else if (addr == 0x7fff)  { if (cartActive()) mapSlot1Write(S_Cartridge, value); }
            else if (addr == 0xbfff) { if (cartActive()) mapSlot2WriteC(value); }
            return;
        case Cart::CODEMASTERS:
            if (addr == 0x0000) { if (cartActive()) mapSlot0Write(S_Cartridge, value); }
            else if (addr == 0x4000) { if (cartActive()) mapSlot1Write(S_Cartridge, value); }
            else if (addr == 0x8000) { if (cartActive()) mapSlot2Write(S_Cartridge, value); }
            return;
        case Cart::KOREAN:
            if (addr == 0xa000) { if (cartActive()) mapSlot2Write(S_Cartridge, value); }
            return;
        case Cart::KOREAN_8K:
        case Cart::KOREAN_8K_NEMESIS:
            if (    (addr == 0x0000)
                ||  (addr == 0x0001)
                ||  (addr == 0x0002)
                ||  (addr == 0x0003) )
                     { if (cartActive()) mapKorean8kWrite(S_Cartridge, addr, value); }
            return;
        case Cart::CUSTOM_JANGGUN:
            if (    (addr == 0x4000)
                ||  (addr == 0x6000)
                ||  (addr == 0x8000)
                ||  (addr == 0xa000) )
                     { if (cartActive()) mapKorean8kWrite(S_Cartridge, addr, value); }
            else if (addr == 0xfffe) {
                if (cartActive()) {
                    slot[S_Cartridge].page1bank = value << 1;
                    slot[S_Cartridge].page2bank = (value << 1) + 1;
                    page1update = true;
                    slot[S_Cartridge].page0flip = value & 0x40;
                    mapSystem(S_Cartridge, value);
                }
            }
            else if (addr == 0xffff) {
                if (cartActive()) {
                    slot[S_Cartridge].page3bank = value << 1;
                    slot[S_Cartridge].page4bank = (value << 1) + 1;
                    page2update = true;
                    slot[S_Cartridge].page1flip = value & 0x40;
                    mapSystem(S_Cartridge, value);
                }
            }
            return;
    }
}

void Bus::writeMemoryControlBios(u32 addr, u8 value) {
    if (sys->getUnit() == _System::GG) {
        if (addr == 0xfffc || addr == 0xfffd) {
            unmap(0x00, 0x03, 0);
            map(0x00, 0x03, *slot[S_Bios].access);
        }
    } else if (sys->getUnit() == _System::SMS) {
        if (addr == 0xfffc) mapRamSelectorWrite(S_Bios, value);
        else if (addr == 0xfffd) mapSlot0Write(S_Bios, value);
        else if (addr == 0xfffe) mapSlot1Write(S_Bios, value);
        else if (addr == 0xffff) mapSlot2Write(S_Bios, value);
    }
}

u8 Bus::MemUnmapped::read(u32 addr) { return 0xff; }

u8 Bus::MemMmio::read(u8 addr) {
	u8 pos = route(addr);
	return _mmio[pos]->mmio_read(pos);
}

void Bus::MemMmio::write(u8 addr, u8 value) {
	u8 pos = route(addr);
	_mmio[pos]->mmio_write(pos, value);
}

void Bus::MemMmio::map(u8 pos, Mmio* access) {
	_mmio[pos] = access;
}

u8 Bus::MemMmio::sgRoute(u8 addr) {
    switch (addr) {
        case 0xc0:
        case 0xdc: return 6;
        case 0xc1:
        case 0xdd: return 7;
        case 0x7e: return 2;
        case 0x7f: return 3;
        case 0xbe: return 4;
        case 0xbf: return 5;
        default: return 15;
    }
}

u8 Bus::MemMmio::smsRoute(u8 addr) {
    if (sys->isYamahaSupported()) {
        if (addr == 0xf0) return 16;
        if (addr == 0xf1) return 17;
        if (addr == 0xf2) return 18;
    }

	switch (addr & 0xc1) {
		case 0x00: return 0; //Memory control -> 0x3e
        case 0x01: return 1; //I/O port control -> 0x3f
		case 0x40: return 2; //V counter, SN76489 data write -> 0x7e
		case 0x41: return 3; //H counter, SN76489 data write -> 0x7f
		case 0x80: return 4; //data port -> 0xBE
		case 0x81: return 5; //control port -> 0xBF
        case 0xc0: {
            if (!sys->bus.isIoDisabled()) {
                return 6; //Joypads Port A + B -> 0xdc
            }
            return 15;
        }
        case 0xc1: {
            if (!sys->bus.isIoDisabled()) {
                return 7; //Joypads Port A + B -> 0xdd
            }
            return 15;
        }
	}
}

u8 Bus::MemMmio::ggRoute(u8 addr) {
    if ( sys->isGGMode() ) { //game gear specific
        switch (addr) {
            case 0x00: return 8;
            case 0x01: return 9;
            case 0x02: return 10;
            case 0x03: return 11;
            case 0x04: return 12;
            case 0x05: return 13;
            case 0x06: return 14;
        }
    }
    switch (addr) {
        case 0xc0:
        case 0xdc: return 6;
        case 0xc1:
        case 0xdd: return 7;
    }
    switch (addr & 0xc1) {
        case 0x00: return 0; //Memory control -> 0x3e
        case 0x01: return sys->isGGMode() ? 15 : 1; //I/O port control -> 0x3f
		case 0x40: return 2; //V counter, SN76489 data write -> 0x7e
		case 0x41: return 3; //H counter, SN76489 data write -> 0x7f
		case 0x80: return 4; //data port -> 0xBE
        case 0x81: return 5; //control port -> 0xBF
	}
	return 15;
}

u8 Bus::MemMmio::route(u8 addr) {
    if (sys->getUnit() == _System::SMS) {
        return smsRoute(addr);
    } if (sys->getUnit() == _System::GG) {
        return ggRoute(addr);
    } if (sys->getUnit() == _System::SG1000) {
        return sgRoute(addr);
    }
}

void Bus::buildFlipTable()
{
    for (unsigned i = 0; i < 256; i++) {
        u8 flip = 0;
        for (unsigned b = 0; b < 8; b++) {
            if ( i & (1<<b) ) flip |= 1 << (7-b);
        }
        flipTable[i] = flip;
    }
}
