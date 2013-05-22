#ifndef COLOR_H
#define COLOR_H

#include "helper.h"
#include "dataTypes.h"

class Filter;

class Color {

public:
    enum Mode { SMS = 0, SMS_TMS = 1, TMS = 2, GG = 3 };
	struct ColorTable {
		u32* _RGB32;
		u16* _RGB16;
	};

private:
	signed contrast;
	signed brightness;
	signed gamma;
	bool colortable_updated;
    ColorTable colorTable[4];
	void adjust_contrast(u8& channel);
	void adjust_gamma(u8& channel);
	void adjust_brightness(u8& channel);
	u8 colorConverter(u8 smColor);

public:
    void update_parameter(signed _contrast, signed _gamma, signed _brightness) {
		contrast = _contrast;
		brightness = _brightness;
		gamma = _gamma;
		colortable_updated = false;
	}

	void update();
	void set_filter(int _type, u8 cable);

	enum FilterType {
		Direct=0,
        NTSC=1
	} type;

	struct {
		u8 green;
		u8 red;
		u8 blue;
	} tms[16];

	struct {
		u8 green;
		u8 red;
		u8 blue;
	} tms_sms[16];

	Filter* active_filter;

	Color() {
		gamma = 100;
		brightness = 0;
		contrast = 0;
		colortable_updated = false;
        colorTable[SMS]._RGB16 = new u16[64];
		colorTable[SMS]._RGB32 = new u32[64];
		colorTable[TMS]._RGB16 = new u16[16];
		colorTable[TMS]._RGB32 = new u32[16];
		colorTable[SMS_TMS]._RGB16 = new u16[16];
        colorTable[SMS_TMS]._RGB32 = new u32[16];
        colorTable[GG]._RGB16 = new u16[4096];
        colorTable[GG]._RGB32 = new u32[4096];
		active_filter = 0;

		tms[0].red = 0; tms[0].green = 0; tms[0].blue = 0;
		tms[1].red = 0; tms[1].green = 0; tms[1].blue = 0;
		tms[2].red = 33; tms[2].green = 200; tms[2].blue = 66;
		tms[3].red = 94; tms[3].green = 220; tms[3].blue = 120;
		tms[4].red = 84; tms[4].green = 85; tms[4].blue = 237;
		tms[5].red = 125; tms[5].green = 118; tms[5].blue = 252;
		tms[6].red = 212; tms[6].green = 82; tms[6].blue = 77;
		tms[7].red = 66; tms[7].green = 235; tms[7].blue = 245;
		tms[8].red = 252; tms[8].green = 85; tms[8].blue = 84;
		tms[9].red = 255; tms[9].green = 121; tms[9].blue = 120;
		tms[10].red = 212; tms[10].green = 193; tms[10].blue = 84;
		tms[11].red = 230; tms[11].green = 206; tms[11].blue = 84;
		tms[12].red = 33; tms[12].green = 176; tms[12].blue = 59;
		tms[13].red = 201; tms[13].green = 91; tms[13].blue = 186;
		tms[14].red = 204; tms[14].green = 204; tms[14].blue = 204;
		tms[15].red = 255; tms[15].green = 255; tms[15].blue = 255;

		//this is taken from meka
		tms_sms[0].red = 0; tms_sms[0].green = 0; tms_sms[0].blue = 0;
		tms_sms[1].red = 0; tms_sms[1].green = 0; tms_sms[1].blue = 0;
		tms_sms[2].red = 4*0x08; tms_sms[2].green = 4*0x30; tms_sms[2].blue = 4*0x08;
		tms_sms[3].red = 4*0x18; tms_sms[3].green = 4*0x38; tms_sms[3].blue = 4*0x18;
		tms_sms[4].red = 4*0x08; tms_sms[4].green = 4*0x08; tms_sms[4].blue = 4*0x38;
		tms_sms[5].red = 4*0x10; tms_sms[5].green = 4*0x18; tms_sms[5].blue = 4*0x38;
		tms_sms[6].red = 4*0x28; tms_sms[6].green = 4*0x08; tms_sms[6].blue = 4*0x08;
		tms_sms[7].red = 4*0x10; tms_sms[7].green = 4*0x30; tms_sms[7].blue = 4*0x38;
		tms_sms[8].red = 4*0x38; tms_sms[8].green = 4*0x08; tms_sms[8].blue = 4*0x08;
		tms_sms[9].red = 4*0x38; tms_sms[9].green = 4*0x18; tms_sms[9].blue = 4*0x18;
		tms_sms[10].red = 4*0x30; tms_sms[10].green = 4*0x30; tms_sms[10].blue = 4*0x08;
		tms_sms[11].red = 4*0x30; tms_sms[11].green = 4*0x30; tms_sms[11].blue = 4*0x20;
		tms_sms[12].red = 4*0x08; tms_sms[12].green = 4*0x20; tms_sms[12].blue = 4*0x08;
		tms_sms[13].red = 4*0x30; tms_sms[13].green = 4*0x10; tms_sms[13].blue = 4*0x28;
		tms_sms[14].red = 4*0x28; tms_sms[14].green = 4*0x28; tms_sms[14].blue = 4*0x28;
		tms_sms[15].red = 4*0x38; tms_sms[15].green = 4*0x38; tms_sms[15].blue = 4*0x38;
	}
	~Color() { 
		free_mem(colorTable[SMS]._RGB16);
		free_mem(colorTable[SMS]._RGB32);
		free_mem(colorTable[TMS]._RGB16);
		free_mem(colorTable[TMS]._RGB32);
		free_mem(colorTable[SMS_TMS]._RGB16);
		free_mem(colorTable[SMS_TMS]._RGB32);
        free_mem(colorTable[GG]._RGB16);
        free_mem(colorTable[GG]._RGB32);
	}
};

class Filter {
public:
	Filter(Color::ColorTable* colorTable) : colorTable(colorTable) {} 
	virtual ~Filter() {}
    virtual void render(Color::Mode mode, u32* gpu_data, u32 gpu_pitch, u16* ppu_data, unsigned width, unsigned height) = 0;
	virtual void size(unsigned& outwidth, unsigned& outheight, unsigned width, unsigned height) = 0;
	virtual void init() = 0;
protected:
	Color::ColorTable* colorTable;
};

class Filter_direct : public Filter {
public:
	Filter_direct(Color::ColorTable* _colorTable) : Filter(_colorTable) {}
	~Filter_direct() {}
    void render(Color::Mode mode, u32* gpu_data, u32 gpu_pitch, u16* ppu_data, unsigned width, unsigned height);
	void size(unsigned& outwidth, unsigned& outheight, unsigned width, unsigned height);
	void init() {}
};

class Filter_ntsc : public Filter {
public:
	Filter_ntsc(Color::ColorTable* _colorTable, u8 _cable);
	~Filter_ntsc();
    void render(Color::Mode mode, u32* gpu_data, u32 gpu_pitch, u16* ppu_data, unsigned width, unsigned height);
	void size(unsigned& outwidth, unsigned& outheight, unsigned width, unsigned height);
	void init();
private:
	enum {composite=0, svideo=1, rgb=2};
	struct sms_ntsc_t *ntsc;
	u8 cable;
    void convertToRGB16(Color::Mode mode, u16* ppu_data, unsigned width, unsigned height);
	u16* frameInRGB16;
};

#endif
