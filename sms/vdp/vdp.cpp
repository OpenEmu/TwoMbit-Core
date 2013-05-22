
#include "vdp.h"
#include "../system/system.h"
#include "../cpu/cpu.h"
#include "../bus/bus.h"

Vdp* Vdp::instance1 = 0;
void Vdp::Enter1() { instance1->enter(); }
Vdp* Vdp::instance2 = 0;
void Vdp::Enter2() { instance2->enter(); }

Vdp::Vdp(_System* sys, Bus& bus, Cpu& cpu, MemStatic& vram)
:
sys(sys),
bus(bus),
cpu(cpu),
vram(vram)
{
    bus.mmio.map(4, this);
    bus.mmio.map(5, this);

    frameBuffer = new u16[284 * 400];
    blackHole = new u16[300];
    vblancLines = 19;
    frameCounter = 0;
}

void Vdp::prepareForMode4() {
    name_address_select = &Vdp::name_address_select_mode4;
    pattern_ram_load = &Vdp::pattern_ram_load_mode4;
    name_ram_load = &Vdp::name_ram_load_mode4;
    color_address_select = &Vdp::color_address_select_mode4;
    pattern_address_select = &Vdp::pattern_address_select_mode4;
    color_ram_load = &Vdp::color_ram_load_mode4;
    spritePreProcessing = &Vdp::spritePreProcessing_mode4;
    calcSprite = &Vdp::calcSprite_mode4;
    checkSpriteOverflow = &Vdp::checkSpriteOverflow_mode4;
    resetStatusReg = &Vdp::resetStatusReg_mode4;
    getBackdrop = &Vdp::getBackdrop_mode4;
    getMode = &Vdp::getMode4;
}

void Vdp::prepareForMode2() {
    name_address_select = &Vdp::name_address_select_mode02;
    pattern_ram_load = &Vdp::pattern_ram_load_mode02;
    name_ram_load = &Vdp::name_ram_load_mode2;
    color_address_select = &Vdp::color_address_select_mode2;
    pattern_address_select = &Vdp::pattern_address_select_mode02;
    color_ram_load = &Vdp::color_ram_load_mode02;
    spritePreProcessing = &Vdp::spritePreProcessing_mode023;
    calcSprite = &Vdp::calcSprite_mode023;
    checkSpriteOverflow = &Vdp::checkSpriteOverflow_mode023;
    resetStatusReg = &Vdp::resetStatusReg_mode023;
    getBackdrop = &Vdp::getBackdrop_mode0123;
    getMode = &Vdp::getMode0123;
}

void Vdp::prepareForMode0() {
    name_address_select = &Vdp::name_address_select_mode02;
    pattern_ram_load = &Vdp::pattern_ram_load_mode02;
    name_ram_load = &Vdp::name_ram_load_mode0;
    color_address_select = &Vdp::color_address_select_mode0;
    pattern_address_select = &Vdp::pattern_address_select_mode02;
    color_ram_load = &Vdp::color_ram_load_mode02;
    spritePreProcessing = &Vdp::spritePreProcessing_mode023;
    calcSprite = &Vdp::calcSprite_mode023;
    checkSpriteOverflow = &Vdp::checkSpriteOverflow_mode023;
    resetStatusReg = &Vdp::resetStatusReg_mode023;
    getBackdrop = &Vdp::getBackdrop_mode0123;
    getMode = &Vdp::getMode0123;
}

void Vdp::calcSprite_mode4(u8 pos) {
    doCalcSpriteMode4(pos*2);
    doCalcSpriteMode4(pos*2 + 1);
}

void Vdp::setLineBuffer() {
    linePtr = noUpdate ? blackHole : frameBuffer + lineBufferCounter * 284;
    lineBufferCounter++;
    linePos = 0;
}

Vdp::VMode Vdp::getVMode( u16 _v ) {
    if (_v < screenHeight ) return DISPLAY;
    if (_v < ( screenHeight + borderBottom ) ) return BORDER_BOTTOM;
    if (_v < ( screenHeight + borderBottom + vblancLines ) ) return BLANKING;
    return BORDER_TOP;
}

Vdp::HMode Vdp::getHMode( u16 _h ) {
    if (_h <= 256 ) return ACTIVE;
    if (_h <= 271 ) return RIGHT;
    if (_h <= 329 ) return HBLANKING;
    return LEFT;
}

void Vdp::enter() {
    while (true) {
        //first 256 VDP cycles
        if (vmode == DISPLAY) buildScreen();
        else {
            for(; _bgCol < 32; _bgCol++) {
                if (fastExit) sys->exit();
                processH(); processH(); processH();
                if ( (vmode != BLANKING) && ((_bgCol % 4) != 0) ) (this->*spritePreProcessing)(); //sprite preprocessing in border top and bottom
                processH(); processH(); processH(); processH(); processH();
            }
            _bgCol = 0;
        }
        //hblanc
        if (vmode != BLANKING) buildSprite();
        else {
            for (u8 i=0; i < 70; i++) {
                processH();
            }
        }
        for (unsigned i=0; i<8; i++) {
            processH();
            if (vmode != BLANKING) (this->*spritePreProcessing)();
            processH();
        }
    }
}

void Vdp::processH() {

    hmode = getHMode( ++hcounter );
    handleCpuAccess();
    if (vmode != BLANKING) {
        if ( (cacheDisplayActive && vmode == DISPLAY) && isSprCol(hcounter) ) {
            statusReg |= 0x20;
        } else if (vmode != DISPLAY && isSprCol(hcounter) && screenHeight == 192 ) {
            statusReg |= 0x20;
        }
    }

    switch (hmode) {
        case ACTIVE:
            if (vmode == BORDER_TOP || vmode == BORDER_BOTTOM) {
                *(linePtr + linePos++) = sys->getDisableBorder() ? 0 : (this->*getBackdrop)();
            }
            break;
        case RIGHT:
            if (vmode != BLANKING) {
                *(linePtr + linePos++) = sys->getDisableBorder() ? 0 : (this->*getBackdrop)();
            }
            if (hcounter == 271 && vmode != BLANKING) setLineBuffer();
            break;
        case HBLANKING:
            if (hcounter == 318) {
                (this->*checkSpriteOverflow)();
                incrementVCounter();
                spriteInit();
                cacheHScroll = regs[8];
                cacheDisplayActive = getBit(regs[1], 6);
                cacheBackDrop = regs[7];
            }
            break;
        case LEFT:
            if (vmode != BLANKING) {
                *(linePtr + linePos++) = sys->getDisableBorder() ? 0 : (this->*getBackdrop)();
            }
            break;
    }
    if (hcounter == 342) hcounter = 0;
    addClocks(1);
}

void Vdp::render() {
    if (vmode == BLANKING) {
        if (!frameComplete) {
            sys->render();
            noUpdate = sys->disable3DEffect() ? !noUpdate : false;            
            frameComplete = true;
        }
    }
}

void Vdp::incrementVCounter() {

    if (++vcounter >= (sys->_isNTSC() ? 262 : 313)  ) { //frame complete
        vcounter = 0;
        frameComplete = false;
        cacheVScroll = regs[9];
        frame();

        if (updateScreenDim) {
            updateScreenDim = false;
            refreshScreenDimension();
        }
    }
    vmode = getVMode( vcounter );
    render();
}

void Vdp::buildScreen() {
    bool customHeight = screenHeight != 192;
    for(; _bgCol < 32; _bgCol++) { //active display (32 * 8) -> vdp cycles
        if (fastExit) sys->exit();
        if ( customHeight && _bgCol == 12 ) cacheDisplayActive = getBit(regs[1], 6);
        if ( !cacheDisplayActive ) {
            for (u8 i=0; i<8; i++) {
                processH();
                if ( (i == 2) && ( (_bgCol % 4) != 0 ) ) (this->*spritePreProcessing)();
                *(linePtr + linePos++) = (this->*getBackdrop)();
            }
            continue;
        }
        processH();
        (this->*name_address_select)(_bgCol);
        processH();
        (this->*pattern_ram_load)();
        processH();
        if ( (_bgCol % 4) != 0 ) (this->*spritePreProcessing)();
        processH();
        (this->*name_ram_load)();
        processH();
        (this->*color_address_select)();
        processH();
        processH();
        (this->*pattern_address_select)(_bgCol);
        processH();
        (this->*color_ram_load)(_bgCol);
    }
    _bgCol = 0;
}

void Vdp::buildSprite() {
    processH();
    processH();
    processH();
    processH();

    for (unsigned i=0; i<4; i++) {
        processH();
        processH();
        processH();
        processH();
        processH();
        processH();

        processH();
        processH();
        processH();
        processH();

        processH();
        processH();
        if (i==0) clearSprCol();
        //todo: split in single operations
        (this->*calcSprite)( i );
        processH();
        processH();
        processH();
    }

    for (unsigned i=0; i<6; i++) {
        processH();
    }
}

void Vdp::spriteInit() {
    spr.seekPos = 0;
    spr.count = 0;
    spr.cancel = false;
    spr.Line = (sys->_isNTSC() ? 262 : 313) == vcounter + 1 ? 0 : vcounter + 1;
    spr.Line = getRealVCounter( spr.Line );
}

void Vdp::frame() {
#ifdef COUNT_ALL_FRAMES
    frameCounter++;
#endif
}

void Vdp::clearSprCache() {
    for (int i = 0; i < 256; i++) {
        pal[i].spr = 16; //transparent
    }
}

void Vdp::calcFrameBorder() {
    //todo: changing screen height is currently processed with the beginning of a new frame. The real machine can do this mid frame, maybe on each scanline?
    vblancLines = 19;
    if (sys->_isNTSC() && screenHeight == 192) {
        borderTop = 27;
        borderBottom = 24;
    } else if (sys->_isNTSC() && screenHeight == 224) {
        borderTop = 11;
        borderBottom = 8;
    } else if (sys->_isNTSC() && screenHeight == 240) {
        //this mode is not working on a ntsc machine.
        borderTop = 6;
        borderBottom = 15;
        vblancLines = 1;
    } else if (!sys->_isNTSC() && screenHeight == 192) {
        borderTop = 54;
        borderBottom = 48;
    } else if (!sys->_isNTSC() && screenHeight == 224) {
        borderTop = 38;
        borderBottom = 32;
    } else if (!sys->_isNTSC() && screenHeight == 240) {
        borderTop = 30;
        borderBottom = 24;
    }
}

void Vdp::refreshScreenDimension() {
    u8 res = 0;
    res |= getBit(regs[0], 2) << 3;
    res |= getBit(regs[1], 3) << 2;
    res |= getBit(regs[0], 1) << 1;
    res |= getBit(regs[1], 4);

    if (sys->getUnit() == _System::SG1000) {
        res &= 7;
    }

    u16 tempHeight = screenHeight;
    screenHeight = 192;

    switch(res) {
        case 0:		screenMode = MODE_0; prepareForMode0(); break; //Graphics 1
        case 1:		screenMode = MODE_1; break; //Text
        case 2:		screenMode = MODE_2; prepareForMode2(); break; //Graphics 2
        case 3:		screenMode = MODE_1_2; break;
        case 4:		screenMode = MODE_3; break; //Multicolor
        case 5:		screenMode = MODE_1_3; break;
        case 6:		screenMode = MODE_2_3; break;
        case 7:		screenMode = MODE_1_2_3; break;
        case 8:		screenMode = MODE_4; prepareForMode4(); break; //Master System
        case 9:		screenMode = MODE_1; break;
        case 10:	screenMode = MODE_4; prepareForMode4(); break;
        case 11:	screenMode = MODE_4_224; prepareForMode4();
                    if (sys->getRevision() != _System::SMS1) {
                        screenHeight = 224;
                    }
                    break;
        case 12:	screenMode = MODE_4; prepareForMode4(); break;
        case 13:	screenMode = MODE_1; break;
        case 14:	screenMode = MODE_4_240; prepareForMode4();
                    if (sys->getRevision() != _System::SMS1) {
                        screenHeight = 240;
                    }
                    break;
        case 15:	screenMode = MODE_4; prepareForMode4(); break;
    }

    if (tempHeight != screenHeight) {
        calcFrameBorder();
    }
}

void Vdp::handleCpuAccess() { //emulating access window
    //are sms and gg access windows the same ?
    //timing diagram for SG VDP describes access windows(VRAM, CRAM ?) during active display are each 32 master, but a few gg games like shining force show glitches
    //therefore set vram/cram access windows the same like register access windows
    //the VDP delay for SMS VDP is smaller (~1,3 µs) than the SG VDP (~2 µs)
    //todo: make a lot of tests on real hardware

    if (access.waitJob != noneAccess) {
        bool isAccess = false;

        if (cacheDisplayActive && hmode == ACTIVE && vmode == DISPLAY) { //active screen
            if (access.waitJob == writeCgRam) {
                if (sys->isGGMode()) {
                    if ( (hcounter % 8) == 0) isAccess = true;
                } else {
                    if ( (hcounter % 32) == 0) isAccess = true;
                    if ( isAccess && hcounter >= 224 ) isAccess = false;
                }
            } else {
                if ( (hcounter >= 3) && ( ( (hcounter - 3) % 16) == 0) ) isAccess = true; //six consecutive "out (C), r" give you a problem, outir is no problem
            }
        } else if ( vmode != BLANKING && hmode != ACTIVE ) { //sprite processing
            if ( (hcounter % 2) == 0 ) { //todo: find out the access windows in this area
                isAccess = true;
            }
        } else {
            isAccess = true;
        }
        if (isAccess) cpu_access();
    }
    updateDelay();
}

void Vdp::initDelay(CpuAccess newjob) {
    access.time = access.maxtime;
    access.prepareJob = newjob;
}

void Vdp::updateDelay() { //emulating VDP delay
    if (access.prepareJob == noneAccess) return;
    if (access.time == 0) return;

    if (--access.time == 0) {
        access.address = commandWord & 0x3FFF;
        if (sys->isGGMode() && access.prepareJob == writeCgRam && ((commandWord & 1) == 0) ) {
            access.latch = dataPort;
            access.prepareJob = noneAccess;
        } else {
            access.waitJob = access.prepareJob;
            access.prepareJob = noneAccess;
        }
        //access.data = dataPort;
        if (access.waitJob != writeReg) {
            u8 cmd = commandWord >> 14;
            commandWord = ( (commandWord+1) & 0x3FFF) | (cmd << 14);
        }
    }
}

void Vdp::cpu_access() {

    switch (access.waitJob) {
        case noneAccess:
            return; //nothing to do
        case readVram:
            dataPort = vram.read( access.address );
            break;
        case writeVram:
            vram.write(access.address, dataPort);
            break;
        case writeCgRam:
            if (sys->isGGMode()) {
                cgRam[(access.address >> 1) & 0x1F] = ( ( dataPort << 8) | access.latch ) & 0xfff;
            } else {
                cgRam[access.address & 0x1F] = dataPort & 0x3f;
            }
            break;
        case writeReg: {
            switch( (commandWord >> 8) & 0xF) {
                default: break;
                case 0:
                    regs[0] = commandWord & 0xFF;
                    cpu.lineirq.activate(getBit(regs[0], 4));
                    updateScreenDim = true;
                    break;
                case 1:
                    regs[1] = commandWord & 0xFF;
                    cpu.frameirq.activate(getBit(regs[1], 5));
                    updateScreenDim = true;
                    break;
                case 2: regs[2] = commandWord & 0xFF; break;
                case 3: regs[3] = commandWord & 0xFF; break;
                case 4: regs[4] = commandWord & 0xFF; break;
                case 5: regs[5] = commandWord & 0xFF; break;
                case 6: regs[6] = commandWord & 0xFF; break;
                case 7: regs[7] = commandWord & 0xFF; break;
                case 8: regs[8] = commandWord & 0xFF; break;
                case 9: regs[9] = commandWord & 0xFF; break;
                case 0xA: regs[0xA] = commandWord & 0xFF; break;
            }
        } break;

    }

    access.waitJob = noneAccess;
}

void Vdp::addClocks(u8 cycles) {
    chipClock += cycles * frequency;
    syncCpu();
}

void Vdp::syncCpu() {
    if(getClock() >= 0) {
        if (sys->getSync() == _System::SYNC_ALL) {
            sys->setSync(_System::SYNC_NORMAL);
            sys->exit();
        }
        co_switch(cpu.getThreadHandle());
    }
}

void Vdp::mmio_write(u8 pos, u8 value) {
    cpu.syncVdp();
    switch(pos) {
        case 4: write_port_BE(value); break;
        case 5: write_port_BF(value); break;
    }
}

u8 Vdp::mmio_read(u8 pos) {
    cpu.syncVdp();
    switch(pos) {
        case 4: return read_port_BE();
        case 5: return read_port_BF();
    }
}

void Vdp::write_port_BE(u8 value) {
    commandSecond = false;
    dataPort = value;

    switch( commandWord >> 14 ) {
        case 0:
        case 1:
        case 2: initDelay(writeVram); break;
        case 3:
        initDelay(writeCgRam);
        if (sys->isGGMode()) {
            access.time = 1;
            updateDelay();
            handleCpuAccess();
        }
        break;
    }
}

void Vdp::write_port_BF(u8 value) {
    if (!commandSecond) {
        commandWord &= 0xFF00 ;
        commandWord |= value ;
    } else {
        commandWord = (value << 8) | (commandWord & 0xFF);

        switch( commandWord >> 14 ) {
            case 0: initDelay(readVram); break;
            case 1: break;
            case 2: access.waitJob = writeReg; break;
            case 3: break;
        }
    }
    commandSecond ^= 1;
}

u8 Vdp::read_port_BE() {
    commandSecond = false;
    initDelay(readVram);
    return dataPort;
}

u8 Vdp::read_port_BF() {
    commandSecond = false;
    u8 res = (this->*resetStatusReg)();
    cpu.frameirq.resetPin();
    cpu.frameirq.resetRegister();
    cpu.lineirq.resetPin();
    cpu.lineirq.resetRegister();
    return res;
}

void Vdp::power() {
    if (sys->getSystemId() == 1 ) {
        create(Enter1, 2);
        instance1 = this;
    } else if (sys->getSystemId() == 2 ) {
        create(Enter2, 2);
        instance2 = this;
    }

    _bgCol = 0;
    fastExit = false;

    sys->getUnit() == _System::SG1000 ? prepareForMode2() : prepareForMode4();
    vram.init(0);
    commandSecond = false;
    dataPort = 0;
    commandBuffer = 0;
    commandWord = 0;
    statusReg = 0;
    access.prepareJob = noneAccess;
    access.waitJob = noneAccess;
    access.latch = 0;
    access.maxtime = sys->getUnit() == _System::SG1000 ? SG_VDP_DELAY : SMS_VDP_DELAY;
    screenHeight = 192;
    screenMode = (sys->getUnit() == _System::SG1000) ? MODE_2 : MODE_4;
    calcFrameBorder();
    vcounter = hcounter = 0;
    frameComplete = false;
    noUpdate = false;

    cacheVScroll = 0;
    cacheHScroll = 0;

    for (int i=0; i < 32; i++) {
        cgRam[i] = 0;
    }
    for (int i=0; i < 0xb; i++) {
        regs[i] = 0;
    }
    regs[0x0] = 0x36;
    regs[0x1] = 0x80;

    regs[0x2] = 0xff;
    regs[0x3] = 0xff;
    regs[0x4] = 0xff;
    regs[0x5] = 0xff;
    regs[0x6] = 0xfb;
    regs[0x7] = 0x00;

    regs[0xa] = 0xff;
    cacheBackDrop = 0;

    clearSprCache();
    spriteInit();

    lineBufferCounter = 0;
    setLineBuffer();
    hmode = ACTIVE;
    updateScreenDim = false;
    cpu.lineirq.activate(getBit(regs[0], 4));
    vmode = getVMode( vcounter );
}

u8 Vdp::getInternalVCounter( u16 v ) {
    if (sys->_isNTSC()) {
        if (screenHeight == 192) {
            return v > 0xDA ? v - 6 : v;
        } else if (screenHeight == 224) {
            return v > 0xEA ? v - 6 : v;
        } else {
            return v > 0xFF ? v - 0x100 : v;
        }
    } else {
        if (screenHeight == 192) {
            return v > 0xF2 ? v - 0x39 : v;
        } else if (screenHeight == 224) {
            if (v > 0xFF) {
                return (v - 0x100) > 0x02 ? v - 0x100 + 0xC7 : v - 0x100;
            }
            return v;
        } else {
            if (v > 0xFF) {
                return (v - 0x100) > 0x0A ? v - 0x100 + 0xC7 : v - 0x100;
            }
            return v;
        }
    }
}

int Vdp::getRealVCounter( u16 v ) {
    if (sys->_isNTSC()) {
        if (screenHeight == 192) {
            return v > 0xda ? v - 262 : v;
        } else if (screenHeight == 224) {
            return v > 0xea ? v - 262 : v;
        } else {
            return v > 0xff ? v - 262 : v;
        }
    } else {
        if (screenHeight == 192) {
            return v > 0xf2 ? v - 313 : v;
        } else if (screenHeight == 224) {
            return v > 0x102 ? v - 313 : v;
        } else {
            return v > 0x10a ? v - 313 : v;
        }
    }
}

