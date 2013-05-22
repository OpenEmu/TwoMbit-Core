
#ifndef _SYSTEM_H
#define _SYSTEM_H

#include "dataTypes.h"

#include "../cart/cart.h"

#include "../cpu/cpu.h"
#include "../bus/bus.h"
#include "../vdp/vdp.h"
#include "../gear2gear/gear2gear.h"
#include "../apu/sn76489.h"
#include "../apu/ym2413.h"
#include "../input/input.h"
#include "../mem.h"

class _Interface;

class _System {

public:
    enum Unit { SMS, GG, SG1000 };
	enum Revision { SMS1 = 0, SMS2 = 1 };
	enum Region { USA = 0, JAPAN = 1, EUROPE = 2, AUTO = 3 };

    enum Synchronisation { SYNC_NORMAL, SYNC_CPU_EDGE, SYNC_ALL } sync;

    Cart cart;
	Bus bus;
	Cpu cpu;
	Vdp vdp;
	SN76489 sn76489;
    YM2413 ym2413;
	_Input input;
	Gear2Gear gear2gear;

    _Interface* interface;

    MemStatic vram;
	MemStatic wram;
	MemStatic wramSG;
	MemSystem rom;
	MemSystem sram;
	MemSystem bios;
    MemSystem cartWram;

    cothread_t getActiveThread() {
        return thread_active;
    }

private:
	Unit unit;
	Revision revision;
	Region region;
    cothread_t thread_app;
	cothread_t thread_active;	//active thread
	bool activateGearToGear;
    bool glassesRom;
    bool disable3D;
    bool yamahaSupported;
    bool disableSmsBorder;
	_System* friendSystem;
	unsigned systemID;
	u16* ggFrameBuffer;
	u16* cropGGFrame();
    u16* smsFrameBuffer;
    void cropSmsFrame(unsigned renderHeight);
    void cropSmsFrameOverscan(unsigned renderHeight);
    void scaleHorizontal(u8 pix);
	u8 getLinePix(unsigned line, unsigned pos);
	u16* scaleGGFrame();
    bool runToSave();
    bool smsMode;
    bool ggMode;

    struct {
        u16* destbuffer;
        u8 index;
    } scaler;

    void gatherSerializationData(Serializer& s);

    unsigned serializationSize;

public:
    void G2Gswitch();
    unsigned getSystemId() { return systemID; }
    void setFriendSystem(_System* _friendSystem) { friendSystem = _friendSystem; }
    void setGlassesRom(bool state) { glassesRom = state; }
    void setDisable3D(bool state) { disable3D = state; }
    void supportYamaha(bool state) { yamahaSupported = state; }
    _System* getFriendSystem() { return friendSystem; }
    bool emulateG2G() { return activateGearToGear && !cart.isGgInSmsMode(); }
    bool isGGMode() { return ggMode; }
    void setGGMode(bool state) { ggMode = state; }
    bool isSmsMode() { return smsMode; }
    void setSmsMode(bool state) { smsMode = state; }
    Unit getUnit() { return unit; }
    bool disable3DEffect() { return !disable3D ? false : glassesRom; }
    bool isYamahaSupported() { return yamahaSupported; }
	void setUnit(Unit _unit) { unit = _unit; }
    bool getRevision() { return unit == GG ? SMS2 : revision; }
	void setRevision(Revision _revision) { revision = _revision; }
    Region _region() { return region; }
    bool _isNTSC() { return region == USA || region == JAPAN || unit == GG; }
    void setRegion(Region _region) { region = _region; }
    void g2gTransfer(u8 bit, bool value, Gear2Gear::Mode _mode );
    unsigned getSerializationSize() {return serializationSize; }
    Synchronisation getSync() { return sync; }
    void setSync(Synchronisation _sync) { sync = _sync; }
    void setDisableSmsBorder(bool state) { disableSmsBorder = state; }
    bool getDisableBorder() { return disableSmsBorder; }

    void power();
    void reset();
	void enter();
	void exit();
    void loadSMS(u8* _rom, u32 _romSize);
    void loadSG1000(u8* _rom, u32 _romSize);
    void loadGG(u8* _rom, u32 _romSize, unsigned ggMode);
    void loadBios(u8* _bios, u32 _biosSize);
    void mapAddCartWram(u8* _mem, u32 _memSize);
	void unload();
	void threadInit();
	void init(_Interface* _interface);
	void render();
	void setPortDevice(_Input::Port port, _Input::DeviceID deviceId);
    void mapCartData(u8* _rom, u32 _romSize, u8* _sram, u32 _sramSize );
	void setRegionFromCart(Region _region);
    bool setCheat(const char* code);
    void resetCheat();
    void serialize(Serializer& s);
    void calcSerializationSize();
    u64 getSerializationTimeStamp(unsigned char* data);
    bool getSerializationData(unsigned char* data, unsigned size);
    bool setSerializationData(unsigned char* data, unsigned size);
    unsigned getLibraryId() {
        return 323005;
    }

    _System(unsigned _systemID);
    ~_System() {
        free_mem(ggFrameBuffer);
        free_mem(smsFrameBuffer);
    }
};

#endif
