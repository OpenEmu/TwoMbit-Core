
#include "vdp.h"
#include "serializer.h"

void Vdp::serialize(Serializer& s) {
    s.integer(_bgCol);
    s.integer(vLine);
    s.integer(lineBufferCounter);
    s.integer(frameComplete);
    s.integer(updateScreenDim);
    s.integer(commandSecond);
    s.integer(commandWord);
    s.integer(commandBuffer);
    s.integer(dataPort);
    s.integer(statusReg);
    s.array(cgRam, 32);
    s.array(regs, 0xb);
    s.integer(nameLatch);
    s.integer(tilePattern);
    s.integer(tileDefinition);
    s.integer(linePos);
    s.integer(cacheVScroll);
    s.integer(cacheHScroll);
    s.integer(cacheDisplayActive);
    s.integer(cacheBackDrop);
    s.integer(patternOffset);
    s.integer(colAddress);

    for (unsigned i = 0; i < 8; i++) {
        s.integer(_tile[i].nibble);
    }
    for (unsigned i = 0; i < 256; i++) {
        s.integer(pal[i].spr);
        s.integer(pal[i].bg);
        s.integer(pal[i].bgPrio);
    }
    s.integer(palCount);
    s.integer(spr.count);
    s.integer(spr.cancel);
    s.integer(spr.seekPos);
    s.integer(spr.Line);
    s.integer(spr.height);
    s.integer(spr.width);
    for (unsigned i = 0; i < 8; i++) {
        s.integer(spr.pos[i]);
        s.integer(spr.tileLine[i]);
    }
    for (unsigned i = 0; i < 256; i++) {
        s.integer(SprCol[i].detected);
    }
    s.integer(vcounter);
    s.integer(hcounter);
    s.integer(borderBottom);
    s.integer(borderTop);
    s.integer(vblancLines);

    s.integer(access.time);

    unsigned _prepareJob = access.prepareJob;
    s.integer(_prepareJob);
    access.prepareJob = (CpuAccess)_prepareJob;

    unsigned _waitJob = access.waitJob;
    s.integer(_waitJob);
    access.waitJob = (CpuAccess)_waitJob;

    unsigned _hmode = hmode;
    s.integer(_hmode);
    hmode = (HMode)_hmode;

    unsigned _vmode = vmode;
    s.integer(_vmode);
    vmode = (VMode)_vmode;

    s.integer(access.address);
    s.integer(access.data);
    s.integer(access.latch);

    s.integer(screenMode);
    s.integer(screenHeight);

    s.integer(chipClock);
}
