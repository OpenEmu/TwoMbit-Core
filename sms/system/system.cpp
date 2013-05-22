
#include "system.h"
#include "interface.h"
#include "../cheats/cheats.h"
#include "serializer.h"

void _System::init(_Interface* _interface) {
    interface = _interface;
}

void _System::threadInit() {
    thread_app = co_active();
    thread_active = cpu.getThreadHandle();
}

void _System::enter() {
    if (!cart.isLoaded()) return;
    if(getSystemId() == 1) {
        interface->pollInput();
        input.update();
    }
    sync = SYNC_NORMAL;
    thread_app = co_active();
    co_switch(thread_active);
}

bool _System::runToSave() {
    bool latchFound = false;
    while(true) {
        vdp.setFastExit(false);
        sync = SYNC_CPU_EDGE;
        thread_active = cpu.getThreadHandle();
        co_switch(thread_active);
        if (sync != SYNC_ALL) {
            break; //no latch point for a whole frame ... give up
        }
        vdp.setFastExit(true);

        thread_active = vdp.getThreadHandle();
        co_switch(thread_active);
        if (sync == SYNC_NORMAL) { //time to pray
            continue; //damn, vdp have to sync too soon ... try again
        }
        latchFound = true;
        break;
    }
    vdp.setFastExit(false);
    return latchFound;
}

void _System::exit() {
    thread_active = co_active();
    co_switch(thread_app);
}

void _System::G2Gswitch() {
    bool coSwitch = false;
    if(getSystemId() == 1) {
        if (Gear2Gear::getSysClock() < 0 ) coSwitch = true;
    } else {
        if (Gear2Gear::getSysClock() >= 0 ) coSwitch = true;
    }

    if (!coSwitch) return;

    thread_active = co_active();
    co_switch ( friendSystem->getActiveThread() );
}

void _System::power() {
    if (!cart.isLoaded()) return;
    bus.power();
	cpu.power();
	vdp.power();
	sn76489.power();
    ym2413.power();
	gear2gear.power();
	input.power();
    threadInit();
}

void _System::render() {
    if(getUnit() == SMS) input.drawCursorIfNeeded();
    u16 _height = vdp.getRenderHeight();
    if(getUnit() != GG) {
        if (disableSmsBorder) {
            cropSmsFrame(_height);
        } else {
            //interface->videoSample(vdp.output(), 284, _height, vdp._getMode(), getSystemId());
            cropSmsFrameOverscan(_height);
        }
    } else {
        if (isGGMode()) {
            interface->videoSample(cropGGFrame(), 160, 144, vdp._getMode(), getSystemId());
        } else {
            u16 screenHeight = vdp.getScreenHeight();
            interface->videoSample(scaleGGFrame(), 160, screenHeight == 192 ? 208 : 240, vdp._getMode(), getSystemId());
        }
    }
}

void _System::cropSmsFrameOverscan(unsigned renderHeight) {
    u16* srcBuffer = vdp.output();
    u16* destBuffer = smsFrameBuffer;

    //renderHeight += 1;
    double overscan = 4.0; // 4 percent
    unsigned removeWidth = unsigned((284.0 * overscan) / 100.0);
    unsigned removeHeight = unsigned(((double)renderHeight * overscan) / 100.0);

    for (unsigned y = removeHeight; y < (renderHeight - removeHeight); y++) {
        unsigned pos = y * 284;
        for (unsigned x = removeWidth; x < (284 - removeWidth); x++) {
            *destBuffer++ = *(srcBuffer + pos + x);
        }
    }
    interface->videoSample(smsFrameBuffer, 284 - 2 * removeWidth, renderHeight - 2 * removeHeight, vdp._getMode(), getSystemId());
}

void _System::cropSmsFrame(unsigned renderHeight) {
    u16* srcBuffer = vdp.output();
    u16* destBuffer = smsFrameBuffer;
    unsigned borderTop = vdp.getBorderTop();
    unsigned borderBottom = vdp.getBorderBottom();
    unsigned screenHeight = vdp.getScreenHeight();
    //renderHeight += 1;

    bool cutWidth = !_isNTSC() || screenHeight == 192;
    unsigned removePix;
    if (cutWidth) {
        removePix = renderHeight - unsigned ((256.0 * (double)renderHeight) / 284.0) ;
    } else {
        removePix = 284 - unsigned ((224.0 * 284.0) / 243.0);
    }
    removePix /= 2;

    unsigned startHeight = cutWidth ? removePix : borderTop;
    unsigned endHeight = (borderTop + screenHeight)
                    + (cutWidth ? borderBottom - removePix : 0);

    unsigned startWidth = cutWidth ? 13 : removePix;
    unsigned endWidth = cutWidth ? 256 + 13 : 284 - removePix;

    for (unsigned y = startHeight; y < endHeight; y++) {
        unsigned pos = y * 284;
        for (unsigned x = startWidth; x < endWidth; x++) {
            *destBuffer++ = *(srcBuffer + pos + x);
        }
    }
    interface->videoSample(smsFrameBuffer, endWidth - startWidth, endHeight - startHeight, vdp._getMode(), getSystemId());
}

u16* _System::cropGGFrame() {
    u16* srcBuffer = vdp.output();
    u16* destBuffer = ggFrameBuffer;
    u16 screenHeight = vdp.getScreenHeight();
    u16 borderTop = vdp.getBorderTop();

    u16 startHeight = borderTop + (screenHeight == 192 ? 23 : 39);

    for (u16 y = startHeight; y < (startHeight + (18*8)); y++) {
        u32 pos = y * 284;
        for (u16 x = 61; x < (61 + 160); x++) {
            *destBuffer++ = *(srcBuffer + pos + x);
        }
    }
    return ggFrameBuffer;
}

void _System::scaleHorizontal(u8 pix) {
    if ( scaler.index == 0 ) {
        *scaler.destbuffer = pix & 0xf;
    } else if (scaler.index == 1) {
        *scaler.destbuffer++ |= ((pix >> 4) & 3) << 4;
        *scaler.destbuffer = pix & 3;
    } else if (scaler.index == 2) {
        *scaler.destbuffer++ |= ((pix >> 2) & 0xf) << 2;
    }
    if (++scaler.index == 3) scaler.index = 0;
}

u8 _System::getLinePix(unsigned line, unsigned pos) {
    u16* srcBuffer = vdp.output();
    unsigned _pos = line * 284 + pos + 8 + 13;
    return *(srcBuffer + _pos) & 0xff;
}

u16* _System::scaleGGFrame() {
    scaler.index = 0;
    scaler.destbuffer = ggFrameBuffer;
    u16 borderTop = vdp.getBorderTop();
    u16 screenHeight = vdp.getScreenHeight();

    unsigned ypos = borderTop;

    ypos -= 8;
    for (unsigned y = 0; y < 8; y++) {
        for (unsigned x = 0; x < 240; x++) {
            scaleHorizontal( getLinePix(ypos, x) );
        }
        ypos++;
    }

    for (unsigned y = 0; y < ( screenHeight == 192 ? 192 : 224 ); y++) {
        for (unsigned x = 0; x < 240; x++) {
            scaleHorizontal( getLinePix(ypos + y, x) );
        }
    }
    ypos += screenHeight;

    for (unsigned y = 0; y < 8; y++) {
        for (unsigned x = 0; x < 240; x++) {
            scaleHorizontal( getLinePix(ypos, x) );
        }
        ypos++;
    }

    return ggFrameBuffer;
}

void _System::reset() {
    if (!cart.isLoaded()) return;
    if (unit == SG1000 || (unit == SMS && revision == SMS1) ) {
        cpu.reset(); //software controlled reset
    } else {
        power(); //others don't have a reset, so do power cycle
    }
}

void _System::mapCartData(u8* _rom, u32 _romSize, u8* _sram, u32 _sramSize ) {
    rom.map(_rom, _romSize);
    sram.map(_sram, _sramSize);
    bios.map(0, 0);
    cartWram.map(0, 0);
}

void _System::mapAddCartWram(u8* _mem, u32 _memSize) {
    cartWram.map(_mem, _memSize);
}

void _System::setRegionFromCart(Region _region) {
    if (region == AUTO) {
        region = _region;
    }
}

void _System::setPortDevice(_Input::Port port, _Input::DeviceID deviceId) {
    input.setPortDevice(port, deviceId);
}

void _System::loadSMS(u8* _rom, u32 _romSize) {
    setUnit(SMS);
    cart.load(_rom, _romSize);
    calcSerializationSize();
}

void _System::loadSG1000(u8* _rom, u32 _romSize) {
    setUnit(SG1000);
    cart.load(_rom, _romSize);
    calcSerializationSize();
}

void _System::loadGG(u8* _rom, u32 _romSize, unsigned ggMode) {
    setUnit(GG);
    cart.load(_rom, _romSize, ggMode == 1);
    activateGearToGear = ggMode == 2;
    if (!activateGearToGear) {
        calcSerializationSize();
    }
}

void _System::loadBios(u8* _bios, u32 _biosSize) {
    bios.map(_bios, _biosSize);
}

void _System::unload() {
    activateGearToGear = false;
    glassesRom = false;
    yamahaSupported = false;
    disable3D = false;
    cart.unload();
}

void _System::g2gTransfer(u8 bit, bool value, Gear2Gear::Mode _mode ) {
    gear2gear._receive(bit, value, _mode);
}

bool _System::setCheat(const char* code) {
    return Cheats::getInstance()->set(code);
}

void _System::resetCheat() {
    Cheats::getInstance()->resetList();
}

void _System::calcSerializationSize() {
    Serializer s;
    unsigned signature = 0;
    u64 seconds = 0;
    s.integer(signature);
    s.integer(seconds);
    gatherSerializationData(s);
    serializationSize = s.size();
}

u64 _System::getSerializationTimeStamp(unsigned char* data) {
    Serializer s(data, serializationSize);
    unsigned signature = 0;
    u64 seconds = 0;
    s.integer(signature);
    s.integer(seconds);
    return seconds;
}

bool _System::getSerializationData(unsigned char* data, unsigned size) {
    if (emulateG2G()) return false;
    if (!cart.isLoaded()) return false;
    if (!runToSave()) {
        return false;
    }
    Serializer s(serializationSize);
    unsigned signature = getLibraryId();
    u64 seconds = time (NULL);
    s.integer(signature);
    s.integer(seconds);
    gatherSerializationData(s);
    if(s.size() > size) return false;
    memcpy(data, s.handle(), s.size());
    return true;
}

bool _System::setSerializationData(unsigned char* data, unsigned size) {
    if (emulateG2G()) return false;
    if (!cart.isLoaded()) return false;
    Serializer s(data, size);
    unsigned signature = 0;
    u64 seconds;
    s.integer(signature);
    s.integer(seconds);

    if (signature != getLibraryId()) {
        return false;
    }
    cpu.create(Cpu::Enter1, 3);
    vdp.create(Vdp::Enter1, 2);
    sn76489.create(SN76489::Enter1, 3);
    ym2413.create(YM2413::Enter, 3);
    gatherSerializationData(s);
    bus.recover();
    vdp.refreshScreenDimension();
    vdp.setFastExit(false);
    thread_active = vdp.getThreadHandle();
    return true;
}

void _System::gatherSerializationData(Serializer& s) {
    serialize(s);
    bus.serialize(s);
    cpu.serialize(s);
    sn76489.serialize(s);
    ym2413.serialize(s);
    vdp.serialize(s);
    input.serialize(s);
}

_System::_System(unsigned _systemID)
:
rom(MemMap::ROM), sram(MemMap::SRAM), bios(MemMap::BIOS), cartWram(MemMap::CARTWRAM),
wram(MemMap::WRAM, 8 * 1024),
wramSG(MemMap::WRAMSG, 1 * 1024),
vram(MemMap::VRAM, 16 * 1024),
cpu(this, bus, vdp, sn76489, ym2413, input, gear2gear),
bus(this, cart, cpu, vdp, wram, wramSG, rom, sram, bios, cartWram),
cart(this, bus),
vdp(this, bus, cpu, vram),
sn76489(this, cpu, bus),
ym2413(this, cpu, bus),
input(this, cpu, vdp),
gear2gear(this, cpu, bus)
{
    systemID = _systemID;
    rom.write_protect(true);
    bios.write_protect(true);
    thread_app = 0;
    thread_active = 0;
    revision = SMS2;
    region = USA;
    ggFrameBuffer = new u16[160 * 240];
    memset(ggFrameBuffer, 0, 160 * 240);
    smsFrameBuffer = new u16[284 * 400];
    memset(smsFrameBuffer, 0, 284 * 400);
    activateGearToGear = false;
    glassesRom = false;
    disable3D = false;
    yamahaSupported = false;
    serializationSize = 0;
    disableSmsBorder = false;
}
