
#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "../vdp/vdp.h"
#include "../input/input.h"

class _Interface {
public:
    virtual void audioSample(signed sampleLeft, signed sampleRight, unsigned systemID) {}
    virtual void videoSample(u16* frame, u16 width, u16 height, Vdp::Mode mode, unsigned systemID) {}
    virtual void pollInput(void) {}
    virtual i16 getInputState(_Input::Port port, _Input::DeviceID deviceId, _Input::IObjectID iobjectId) { return 1; }
};

#endif
