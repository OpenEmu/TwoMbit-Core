#include "vdp.h"
#include "../system/system.h"

void Vdp::name_address_select_mode02(u8 col) {
	u8 row = vcounter >> 3;
	nameLatch = ((regs[2] & 0xf) << 10) + ( row << 5 ) + col;
}

void Vdp::pattern_ram_load_mode02() {
	tilePattern = vram[ nameLatch ];
}

void Vdp::name_ram_load_mode2() {
	u8 row = vcounter >> 3;
	u8 line = vcounter % 8;

	u16 nameBaseAdr = getBit(regs[4], 2) ? 0x2000 : 0;
	nameBaseAdr += tilePattern << 3;
	u8 patternTable = 0;
	if (row > 7) {
		if (row > 15) {
			if (getBit(regs[4], 1)) patternTable = 2;
		} else {
			if (getBit(regs[4], 0)) patternTable = 1;
		}
	}
	patternOffset = 0;
	if (patternTable == 1) {
		patternOffset = 256 * 8;
	} else if (patternTable == 2) {
		patternOffset = 256 * 2 * 8;
	}
	nameBaseAdr += patternOffset + line;
	tileDefinition = vram[ nameBaseAdr ];
}

void Vdp::color_address_select_mode2() {
	u8 line = vcounter % 8;
	u8 colAnd = regs[3] & 127;
	colAnd <<= 1;
	colAnd |= 1;
	u8 colIndex = tilePattern & colAnd;
	colAddress = (getBit(regs[3], 7) ? 0x2000 : 0) + (colIndex << 3) + patternOffset + line;
}

void Vdp::pattern_address_select_mode02(u8 col) {
	for (u8 i = 0; i < 8; i++) {
		u8 pos = ( col << 3 ) + i;
		pal[pos].bg = getBit(tileDefinition & 0xff, ~i & 7) ? 1 : 0;
	}
}

void Vdp::color_ram_load_mode02(u8 col) {
	u8 color = vram[ colAddress ];
	u8 fore = color >> 4;
	u8 back = color & 0xF;
	u8 useColor;
	u8 i;
	for (i = 0; i < 8; i++) {
		u8 pos = ( col << 3 ) + i;
		if ( pal[pos].spr != 16 && pal[pos].spr != 0 ) {
			useColor = pal[pos].spr;
		} else {
			useColor = pal[pos].bg == 1 ? fore : back;
		}
        if ( useColor == 0 ) useColor = cacheBackDrop & 0x0F;
		*(linePtr + linePos++) = useColor;
	}

	if (getBit(regs[0], 5) && (col == 0 ) ) {
		for (i = 0; i < 8; i++) {
            *(linePtr + 13 + i) = cacheBackDrop & 0x0F;
		}
	}
}

void Vdp::spritePreProcessing_mode023() {
	if (spr.cancel || spr.count == 5) return;
    spr.ptr = (u8*)&vram[ getSprBaseLegacy() ];
	int yp = spr.ptr[spr.seekPos * 4];
	if (yp++ == 208) {
		spr.cancel = true;
		return;
	}
	bool zoomed = getBit(regs[1], 0);
	spr.height = getBit(regs[1], 1) ? 16 : 8;
	if( zoomed ) {
		spr.height <<= 1;
	}

	if(yp > 224) yp -= 256;

	if (( (spr.Line) >= yp) && ( (spr.Line ) < (yp + spr.height))) {
		if (++spr.count == 5) return;
		spr.pos[spr.count-1] = spr.seekPos;
		spr.tileLine[spr.count-1] = (spr.Line - yp) >> zoomed;
	}
	if (++spr.seekPos == 32) {
		spr.seekPos--;
	}
}

void Vdp::calcSprite_mode023(u8 pos) {
    //if (!cacheDisplayActive) return;
	if (pos == 0) clearSprCache();
	if (spr.count <= pos) return;
	u8 sprPos = spr.pos[pos];
    spr.ptr = (u8*)&vram[ getSprBaseLegacy() ];
	int xp = spr.ptr[sprPos * 4 + 1];
	u8 pattern = spr.ptr[sprPos * 4 + 2];
	u8 color = spr.ptr[sprPos * 4 + 3];
	bool ec = getBit(color, 7);
	color &= 0xF;
	//if ( color == 0 ) return;
	if ( ec ) xp -= 32;
	bool doubleSize = getBit(regs[1], 1);
	bool zoomed = getBit(regs[1], 0);
	spr.width = doubleSize ? 16 : 8;
	if( zoomed ) spr.width <<= 1;

	u16 sgtable = (regs[0x6] & 7) << 11;
	u16 adr = sgtable + ( pattern << 3 );
	if ( doubleSize ) adr = sgtable + ( ( pattern & 252 ) << 3 );

	u8 drawLine = vram[ adr + spr.tileLine[pos] ];
	drawLineOriginalTMSSprite( xp, drawLine, color, zoomed );

	if ( doubleSize ) {
		drawLine = vram[ adr + spr.tileLine[pos] + 16 ];
		drawLineOriginalTMSSprite( xp + (zoomed ? 16 : 8), drawLine, color, zoomed );
	}
}

void Vdp::drawLineOriginalTMSSprite(int xp, u8 drawLine, u8 color, bool zoomed) {
	u8 invert;
	for (u8 x = 0; x < (zoomed ? 16 : 8); x++) {
		if ( ( (xp + x) < 0 ) || ( ( xp + x ) > 255 ) ) continue; //offscreen

		invert = ~x & (zoomed ? 15 : 7);
		if ( zoomed ) invert /= 2;

		if ( !getBit(drawLine, invert) ) {
			continue; //transparent
		}

		if (pal[ xp + x].spr == 16) {
			pal[ xp + x].spr = color;
		} else {
			setSprCol( xp + x );
		}
	}
}

void Vdp::checkSpriteOverflow_mode023() {
    if ( vmode != DISPLAY ) return;
	statusReg &= 0xe0;
	statusReg |= spr.seekPos;

	if ( spr.count == 5 ) {
		statusReg |= 0x40;
	} else {
		statusReg &= ~0x40;
	}
}

u8 Vdp::resetStatusReg_mode023() {
	u8 res = statusReg;
	statusReg &= 0x5F;
	return res;
}

u16 Vdp::getBackdrop_mode0123() {
    return cacheBackDrop & 0x0F;
}

Vdp::Mode Vdp::getMode0123() {
	return sys->getUnit() == _System::SG1000 ? TMS : SMS_TMS;
}
