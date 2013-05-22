#include "color.h"

void Filter_direct::render(Color::Mode mode, u32* gpu_data, u32 gpu_pitch, u16* ppu_data, unsigned width, unsigned height) {
    gpu_pitch >>= 2;
	
    for (unsigned y=0; y < height+1; y++) {
        if (y == height) {
            ppu_data -= width; //repeat last line for linear filter
        }
        for(unsigned x = 0; x < width; x++) {
            *gpu_data++ = colorTable[mode]._RGB32[*ppu_data++];
		}
        *gpu_data = colorTable[mode]._RGB32[*(ppu_data-1)]; //repeat last pixel for filter
        gpu_data += gpu_pitch - width;
	}

}

void Filter_direct::size(unsigned& outwidth, unsigned& outheight, unsigned width, unsigned height) {
	outwidth  = width;
	outheight = height;
}
