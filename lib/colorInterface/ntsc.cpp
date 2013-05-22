#include "color.h"
#include "sms_ntsc.h"

Filter_ntsc::Filter_ntsc(Color::ColorTable* _colorTable, u8 _cable) : Filter(_colorTable) {
	ntsc = 0;
	cable = _cable;
	frameInRGB16 = new u16[ 284 * 400 ];
}

Filter_ntsc::~Filter_ntsc() { 
	if(ntsc) delete(ntsc);
	free_mem( frameInRGB16 );
}

void Filter_ntsc::init() {
	sms_ntsc_setup_t setup;

	switch (cable) {
		case composite: setup = sms_ntsc_composite; break;
		case svideo: setup = sms_ntsc_svideo; break;
		case rgb: setup = sms_ntsc_rgb; break;
	}

	ntsc = new sms_ntsc_t[sizeof ntsc];
	sms_ntsc_init(ntsc, &setup);
}

void Filter_ntsc::convertToRGB16(Color::Mode mode, u16* ppu_data, unsigned width, unsigned height) {
	u16* buffer = frameInRGB16;
	for (unsigned y=0; y < height; y++) {
        for(unsigned x = 0; x < width; x++) {
			*buffer++ = colorTable[mode]._RGB16[*ppu_data++];
		}
	}
}

void Filter_ntsc::render(Color::Mode mode, u32* gpu_data, u32 gpu_pitch, u16* ppu_data, unsigned width, unsigned height) {
	if(!ntsc) return;
    convertToRGB16(mode, ppu_data, width, height);
    sms_ntsc_blit( ntsc, frameInRGB16, width, width, height, gpu_data, gpu_pitch );
}

void Filter_ntsc::size(unsigned& outwidth, unsigned& outheight, unsigned width, unsigned height) {
    outwidth  = SMS_NTSC_OUT_WIDTH(width);
	outheight = height;
}

