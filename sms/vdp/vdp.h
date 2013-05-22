#ifndef VDP_H
#define VDP_H

//#define COUNT_ALL_FRAMES

#include "dataTypes.h"
#include "../processor.h"
#include "../mem.h"

class _System;
class Cpu;
class Bus;
class Serializer;

class Vdp : public Mmio, public Processor {
	_System* sys;
	Cpu& cpu;
	Bus& bus;

	static Vdp* instance1;
	static Vdp* instance2;
    void syncCpu();
	MemStatic& vram;
public:
	enum VMode { DISPLAY, BORDER_BOTTOM, BLANKING, BORDER_TOP } vmode;
	enum HMode { ACTIVE, RIGHT, HBLANKING, LEFT } hmode;
	enum Mode { SMS = 0, SMS_TMS = 1, TMS = 2, GG = 3 };
private:
    unsigned _bgCol;
    bool fastExit;
    enum { SMS_VDP_DELAY = 7, SG_VDP_DELAY = 11 };
	void (Vdp::*name_address_select)(u8 col);
	void (Vdp::*pattern_ram_load)();
	void (Vdp::*name_ram_load)();
	void (Vdp::*color_address_select)();
	void (Vdp::*pattern_address_select)(u8 col);
	void (Vdp::*color_ram_load)(u8 col);
	void (Vdp::*spritePreProcessing)();
	void (Vdp::*calcSprite)(u8 pos);
	void (Vdp::*checkSpriteOverflow)();
	u8 (Vdp::*resetStatusReg)();
    u16 (Vdp::*getBackdrop)();
	Mode (Vdp::*getMode)();

	u16* frameBuffer;
	u16* linePtr;
	u16 vLine;

	u16 lineBufferCounter;
	bool frameComplete;
    bool noUpdate;
    u16* blackHole;

	u64 frameCounter;

    #define BACKDROP_COLOR (0x10 | (cacheBackDrop & 0x0F))

	void buildScreen();
	void buildSprite();
	void write_port_BE(u8 value);
	void write_port_BF(u8 value);

	u8 read_port_BE();
	u8 read_port_BF();

	bool updateScreenDim;

	bool commandSecond;
	u16 commandWord;
	u8 commandBuffer;
	u8 dataPort;
	u8 statusReg;

	u16 cgRam[32];

    u8 regs[0xb];

	u16 nameLatch;
	u16 tilePattern;
	u32 tileDefinition;
	u16 linePos;
	u8 cacheVScroll;
	u8 cacheHScroll;
	bool cacheDisplayActive;
	u8 cacheBackDrop;
	u16 patternOffset;
	u16 colAddress;

    struct {
        u8 nibble;
    } _tile[8];

	struct {
		u8 spr;
		u8 bg;
		bool bgPrio;
	} pal[256];
	u16 palCount;

	struct {
		u8 count;
		bool cancel;
		u8 seekPos;
		int Line;
        u8* ptr;
		u8 height;
		u8 width;
		u8 pos[8];
		u8 tileLine[8];
	} spr;

	struct {
		bool detected;
	} SprCol[256];

	void clearSprCol() {
		for(u16 i=0; i < 256; i++) {
			SprCol[i].detected = false;
		}
	}

	bool isSprCol( u16 pos ) {
        u8 helper = 12; // 0 - 12 left border
		if (pos < helper) return false;
		if (pos > (255+helper)) return false;
		return SprCol[(pos-helper) & 255].detected;
	}
	void setSprCol( u16 pos ) {
		SprCol[pos & 255].detected = true;
	}

	void refresh_screen_mode();

	u16 vcounter;
	u16 hcounter;

	u8 borderBottom;
	u8 borderTop;
	u8 vblancLines;

	enum CpuAccess { noneAccess, readVram, writeVram, writeCgRam, writeReg };

	struct {
		u8 time;
        unsigned maxtime;
		CpuAccess prepareJob;
		CpuAccess waitJob;
		u16 address;
		u8 data;
        u8 latch;
	} access;

	void processH();

	u8 screenMode;
	u8 screenHeight;
	enum { MODE_0, MODE_1, MODE_2, MODE_1_2, MODE_3, MODE_1_3, MODE_2_3, MODE_1_2_3, MODE_4, MODE_4_224, MODE_4_240 };

	bool getBit(u8 reg, u8 bit) {
		return (reg >> bit) & 1;
	}

    void calcPixelFromTileLine(u32 srcLine);
	u8 getPixelFromTileLine(u8 pix, u32 srcLine);
	void calcSprite_mode023(u8 pos);
	void calcSprite_mode4(u8 pos);
	void doCalcSpriteMode4(u8 pos);
	void clearSprCache();
	void cpu_access();
	void handleCpuAccess();
	void updateDelay();
	void initDelay(CpuAccess newjob);

	u16 latchBgBase();
	void pattern_ram_load_mode02();
	void pattern_ram_load_mode4();
	void name_ram_load_mode0();
	void name_ram_load_mode2();
	void name_ram_load_mode4();
	void name_address_select_mode02(u8 col);
	void name_address_select_mode4(u8 col);
	void color_address_select_mode0();
	void color_address_select_mode2();
	void color_address_select_mode4();
	void pattern_address_select_mode02(u8 col);
	void pattern_address_select_mode4(u8 col);
	void color_ram_load_mode02(u8 col);
	void color_ram_load_mode4(u8 col);
	void checkSpriteOverflow_mode023();
	void checkSpriteOverflow_mode4();
	u8 resetStatusReg_mode023();
	u8 resetStatusReg_mode4();
    u16 getBackdrop_mode0123();
    u16 getBackdrop_mode4();
	Mode getMode0123();
	Mode getMode4();
	void drawLineOriginalTMSSprite(int xp, u8 drawLine, u8 color, bool zoomed);

	void prepareForMode4();
	void prepareForMode2();
	void prepareForMode0();

	u16 getSprBase() {
		return ( regs[5] << 7) & 0x3F00;
	}
	u16 getSprBaseLegacy() {
		return ( regs[5] << 7) & 0x3F80;
	}

	void spritePreProcessing_mode4();
	void spritePreProcessing_mode023();
	void spriteInit();

	void addClocks(u8 cycles);
	void setLineBuffer();
	void frame();

	void calcFrameBorder();
	void incrementVCounter();
    void render();

public:
	Vdp(_System* sys, Bus& bus, Cpu& cpu, MemStatic& vram);
    ~Vdp() {
        free_mem(frameBuffer);
        free_mem(blackHole);
    }

    VMode getVMode( u16 _v );
	HMode getHMode( u16 _h );

	void enter();
    void enterState();
    static void Enter1();
	static void Enter2();
	u8 mmio_read(u8 pos);
	void mmio_write(u8 pos, u8 value);

	void power();
    void refreshScreenDimension();

	u16* output() {
		return frameBuffer;
	}

	void debugLook() {}
	u8 getInternalVCounter( u16 v );
	int getRealVCounter( u16 v );

	u16 getRenderHeight() {
		u16 out = lineBufferCounter;
		lineBufferCounter = 0;
		linePos = 0;
        return out;
	}

	void setIrqState() {
		statusReg |= 0x80;
	}

	u8 getScreenHeight() {
		return screenHeight;
	}

	u8 getLineCounter() {
		return regs[0xA];
	}

	Mode _getMode() {
		return (this->*getMode)();
	}

	u16 getDisplayHeight() {
        return screenHeight + borderTop + borderBottom;
	}

	u8 getBorderTop() {
        return borderTop;
	}

    u8 getBorderBottom() {
        return borderBottom;
    }

    u16 getHCount() {
        return hcounter;
    }

    void setFastExit(bool state) {
        fastExit = state;
    }

    void serialize(Serializer& s);
};

#endif

