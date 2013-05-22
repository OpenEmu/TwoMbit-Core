#include "vdp.h"
#include "../system/system.h"

u16 Vdp::latchBgBase() {
	if (screenMode == MODE_4_224 || screenMode == MODE_4_240) {
		return ( ( regs[2] << 10) & 0x3000) | 0x0700;
	}
	return ( regs[2] << 10) & 0x3800;
}

void Vdp::name_address_select_mode4(u8 col) {
	if (col == 0) palCount = 0;
	u16 baseAddress = latchBgBase();
	u16 yScrollMask = screenHeight == 192 ? 224 : 256;
	vLine = (vcounter + cacheVScroll) % yScrollMask;
	if ( getBit(regs[0], 7) && col >= 24) {
		vLine = vcounter;
		baseAddress = (regs[2] << 10) & 0x3800;
	}

	u16 hScroll = getBit(regs[0], 6) && (vcounter < 0x10) ? 0 : 0x100 - cacheHScroll;
	u8 numberTilesScroll = hScroll >> 3;

	u8 shift = hScroll & 7;
	if (shift) {
		if (col == 0) {
			for(int x = shift; x < 8; x++) {
                pal[palCount].bg = BACKDROP_COLOR;
				pal[palCount++].bgPrio = false;
			}
		}
		col++;
	}

	u8 row = vLine >> 3;
	if (sys->getRevision() == _System::SMS1) {
		row &= (getBit(regs[2], 0) << 4) | 0xF;
	}
	nameLatch = baseAddress + ( row << 6 );
	nameLatch += (((col + numberTilesScroll) & 0x1F) << 1);
}

void Vdp::pattern_ram_load_mode4() {
	tilePattern = ( vram[ nameLatch + 1 ] << 8 ) | vram[ nameLatch ];
}

void Vdp::name_ram_load_mode4() {
	u8 tileYpos = vLine & 7;
	bool vInvert = (tilePattern >> 10) & 1;
	if (vInvert) {
		tileYpos = ~tileYpos & 7;
	}
	u16 tileData = tilePattern & 0x1FF;

	u32 vPos = (tileData << 5) + (tileYpos << 2) ;

	tileDefinition = (vram[ vPos ] << 0) | (vram[ vPos + 1 ] << 8) | (vram[ vPos + 2 ] << 16) | (vram[ vPos + 3 ] << 24);
}

u8 Vdp::getPixelFromTileLine(u8 pix, u32 srcLine) {
	pix = ~pix & 7;
	u8 _pal = 0;
	_pal |= ((srcLine >> pix) & 1) << 0;
	_pal |= (( (srcLine >> 8) >> pix) & 1) << 1;
	_pal |= (( (srcLine >> 16) >> pix) & 1) << 2;
	_pal |= (( (srcLine >> 24) >> pix) & 1) << 3;

	return _pal;
}

void Vdp::calcPixelFromTileLine(u32 srcLine) {
    unsigned nib1 = srcLine & 0xff;
    unsigned nib2 = (srcLine >> 8) & 0xff;
    unsigned nib3 = (srcLine >> 16) & 0xff;
    unsigned nib4 = (srcLine >> 24) & 0xff;

    for( unsigned i = 7; ; --i ) {
        _tile[7-i].nibble = ((nib1 >> i) & 1)
                            | (( nib2 >> i) & 1) << 1
                            | (( nib3 >> i) & 1) << 2
                            | (( nib4 >> i) & 1) << 3;
        if (i==0) return;
    }
}

void Vdp::color_address_select_mode4() {
    if (palCount == 256) return;
	bool hInvert = (tilePattern >> 9) & 1;
	bool useSpritePalette = (tilePattern >> 11) & 1;
	bool hasPriority = (tilePattern >> 12) & 1;

	u8 bit;
    calcPixelFromTileLine(tileDefinition);

    for (u8 i = 0; i<8; i++) {
		bit = hInvert ? ~i & 7 : i;
        //pal[palCount].bg = getPixelFromTileLine(bit, tileDefinition);
        pal[palCount].bg = _tile[bit].nibble;

		if (useSpritePalette) {
			pal[palCount].bg += 16;
		}
		pal[palCount++].bgPrio = hasPriority;
		if (palCount == 256) return;
	}
}

void Vdp::pattern_address_select_mode4(u8 col) {
	u16 pos;
    for (u8 i = 0; i < 8; i++) {
		pos = ( col << 3 ) + i;

		if ( !pal[pos].bgPrio || pal[pos].bg == 0 || pal[pos].bg == 16 ) { //bg has no prio
			if (pal[pos].spr != 16) {
				pal[pos].bg = pal[pos].spr;
			}
		}
	}
}

void Vdp::color_ram_load_mode4(u8 col) {
	u8 i;
	u16 pos;
	for (i = 0; i < 8; i++) {
		pos = ( col << 3 ) + i;
		*(linePtr + linePos++) = cgRam[ pal[pos].bg ];
	}
    if (sys->getUnit() == _System::GG) return;
	if (getBit(regs[0], 5) && (col == 0 ) ) {
		for (i = 0; i < 8; i++) {
            *(linePtr + 13 + i) = cgRam[ BACKDROP_COLOR ];
		}
	}
}

void Vdp::spritePreProcessing_mode4() {
	if (spr.cancel || spr.count == 9) return;
    spr.ptr = (u8*)&vram[ getSprBase() ];
	for (u8 i=0; i<2; i++) {
		int yp = spr.ptr[spr.seekPos];
		if (screenHeight == 192 && yp == 208) {
			spr.cancel = true;
			return;
		}
		yp++;
		bool zoomed = getBit(regs[1], 0);
		spr.height = getBit(regs[1], 1) ? 16 : 8;
		if( zoomed ) {
			spr.height <<= 1;
		}

		if(yp > 240) yp -= 256;

		if (( spr.Line >= yp) && ( spr.Line < (yp + spr.height))) {
			if (++spr.count == 9) return;
			spr.pos[spr.count-1] = spr.seekPos;
			spr.tileLine[spr.count-1] = (spr.Line - yp) >> zoomed;
		}
		spr.seekPos++;
	}
}

void Vdp::doCalcSpriteMode4(u8 pos) {
    //if (!cacheDisplayActive) return;
	if (pos == 0) clearSprCache();
	if(spr.count <= pos) return;
	u8 sprPos = spr.pos[pos];

	spr.width = 8;
	if( getBit(regs[1], 0)) {
		spr.width <<= 1;
    }

	u8 start = 0;
	u8 end = spr.width;

	u8 _temp = 0x80;
	if (sys->getRevision() == _System::SMS1) {
		if (!getBit(regs[5], 0)) _temp = 0;
	}

    spr.ptr = (u8*)&vram[ getSprBase() ];
	int xp = spr.ptr[ _temp + (sprPos << 1)];
    u16 n = spr.ptr[ (_temp + 1) + (sprPos << 1)];

    if (getBit(regs[0], 3)) xp -= 8;
	if (getBit(regs[6], 2)) n |= 0x0100;
    if (getBit(regs[1], 1)) n &= 0x01FE;

	if (xp < 0) {
		start = 0 - xp;
	}
    if ( (xp + spr.width) > 256) {
		end = 256 - xp;
	}

	int c = (n & 0x01FF) << 5;
	c += spr.tileLine[pos] << 2;

	if (sys->getRevision() == _System::SMS1) {
		c &= (((getBit(regs[6], 1) << 1) | getBit(regs[6], 0)) << 11) | 0x27ff;
	}

	u32 palLine = (vram[ c ] << 0) | (vram[ c + 1 ] << 8) | (vram[ c + 2 ] << 16) | (vram[ c + 3 ] << 24);
    calcPixelFromTileLine(palLine);
    u8 _pal;

	for(u16 x = start; x < end; x++) {
		u16 _x = x;
		if( getBit(regs[1], 0)) _x = u16(x / 2);
        //u8 _pal = getPixelFromTileLine(_x, palLine);
        _pal = _tile[_x].nibble;

		if (_pal == 0) continue;

		if (pal[ xp + x].spr == 16) {
			pal[ xp + x].spr = _pal + 16;
		} else {
			setSprCol( xp + x );
		}
    }
}

void Vdp::checkSpriteOverflow_mode4() {
	if(spr.count == 9 && vmode == DISPLAY) statusReg |= 0x40;
}

u8 Vdp::resetStatusReg_mode4() {
	u8 res = statusReg;
	statusReg &= 0x1F;
	return res | 0x1F;
}

u16 Vdp::getBackdrop_mode4() {
    return cgRam[BACKDROP_COLOR];
}

Vdp::Mode Vdp::getMode4() {
    return sys->isGGMode() ? GG : SMS;
}
