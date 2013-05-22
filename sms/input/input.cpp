
#include "input.h"
#include "../system/system.h"
#include "../cpu/cpu.h"
#include "../vdp/vdp.h"
#include "../system/interface.h"

u8 _Input::readPort(_Input::Port port, _Input::IObjectID iobjectId) {
	if (port != 0 && port != 1) return 1;
    SmsPort& p = smsport[port];
    if (p.deviceId == 0) return 1;
    DeviceID _dev = p.deviceId;

    if (sys->getUnit() != _System::SMS) {
        _dev = IdJoypad;
    }

    switch(_dev) {
		case IdJoypad:
            return !( sys->interface->getInputState(port, IdJoypad, iobjectId) & 1 );
        case IdLightgun:
            return !( sys->interface->getInputState(port, IdLightgun, _Input::Trigger) & 1 );
        case IdPaddle:
            return readPaddle( port, iobjectId);
        case IdSportsPad:
            if (sys->_region() == _System::JAPAN) return readSportsPadJap( port, iobjectId );
            return readSportsPad( port, iobjectId );
	}
	return 1; //not pressed
}

void _Input::setPortDevice(_Input::Port port, _Input::DeviceID deviceId) {
    if (port != 0 && port != 1) return;
    smsport[port].deviceId = deviceId;
    cpu.getBus().setTerebi(smsport[Port1].deviceId == IdTerebi);
    setCustomInput();
}

void _Input::setCustomInput() {
    useCustomInput = false;
    for (u8 port=0; port<=1; port++) {
        SmsPort& p = smsport[port];
        if ( p.deviceId == IdLightgun || p.deviceId == IdPaddle || p.deviceId == IdSportsPad) {
            useCustomInput = true;
            return;
        }
    }
}

u8 _Input::readPaddle( _Input::Port port, _Input::IObjectID iobjectId) {
    if (iobjectId == _Input::A) {
        return !( sys->interface->getInputState(port, _Input::IdPaddle, iobjectId) & 1 );
    } else if (iobjectId == _Input::B) {
        if (sys->_region() == _System::JAPAN) {
            return paddle[port].flip;
        } else {
            return _getBit( cpu.getPortDD(), 6);
        }
    } else {
        u8 nibble;
        if (sys->_region() == _System::JAPAN) {
            nibble = paddle[port].flip ? 4 : 0;
        } else {
            nibble = _getBit( cpu.getPortDD(), 6) ? 4 : 0;
        }

        if (iobjectId == _Input::Up) {
            return (paddle[port].pos >> (0 + nibble)) & 1;
        } if (iobjectId == _Input::Down) {
            return (paddle[port].pos >> (1 + nibble)) & 1;
        } if (iobjectId == _Input::Left) {
            return (paddle[port].pos >> (2 + nibble)) & 1;
        } if (iobjectId == _Input::Right) {
            return (paddle[port].pos >> (3 + nibble)) & 1;
        }
    }
    return 1;
}

void _Input::updatePaddle() {
    if (paddle[Port1].counter++ == 149) {
        paddle[Port1].counter = 0;
        paddle[Port1].flip ^= 1;
    }
    if (paddle[Port2].counter++ == 149) {
        paddle[Port2].counter = 0;
        paddle[Port2].flip ^= 1;
    }
}

void _Input::updateSportspad(u8 _old, u8 _new) {
    if ((_old & 0xf) != (_new & 0xf)) {
        sportspad[Port1].latch = sportspad[Port2].latch = 1;
    }
    if (_new == 0x0d) //port1 th is output
        sportspad[Port1].latch ^= 1;
    if (_new == 0x07) //port2 th is output
        sportspad[Port2].latch ^= 1;
}

u8 _Input::readSportsPad( _Input::Port port, _Input::IObjectID iobjectId) {

    if (iobjectId == _Input::A) {
        return !( sys->interface->getInputState(port, _Input::IdSportsPad, iobjectId) & 1 );
    } else if (iobjectId == _Input::B) {
        return !( sys->interface->getInputState(port, _Input::IdSportsPad, iobjectId) & 1 );
    } else {
        u8 nibble;
        u8 useDirection = sportspad[port].latch ? sportspad[port].posY : sportspad[port].posX;
        if (port == Port1) {
            if (cpu.getPort3F() == 0x0d) {
                nibble = useDirection >> 4;
            } else if (cpu.getPort3F() == 0x2d) {
                nibble = useDirection;
            } else {
                nibble = cpu.getPortDC();
            }
        } else if (port == Port2) {
            if (cpu.getPort3F() == 0x07) {
                nibble = useDirection >> 4;
            } else if (cpu.getPort3F() == 0x87) {
                nibble = useDirection;
            } else {
                nibble = (cpu.getPortDC() >> 6) | (cpu.getPortDD() << 2);
            }
        }

        if (iobjectId == _Input::Up) {
            return (nibble >> 0) & 1;
        } if (iobjectId == _Input::Down) {
            return (nibble >> 1) & 1;
        } if (iobjectId == _Input::Left) {
            return (nibble >> 2) & 1;
        } if (iobjectId == _Input::Right) {
            return (nibble >> 3) & 1;
        }
    }
    return 1;
}

void _Input::tickJapSportspad(_Input::Port port) {

    if ( sportspad[port].clock++ >= 190) {
        sportspad[port].clock = 0;
        if (++sportspad[port].strobeNibble == 5) {
            sportspad[port].strobeNibble = 0;
        }
    }
}

u8 _Input::readSportsPadJap(_Input::Port port,_Input::IObjectID iobjectId) {
    u8 nibble = 0xff;
    u8 _a, _b, _x, _y;
    bool bit4 = 1;
    bool bit5 = 1;
    switch(sportspad[port].strobeNibble) {
        case 0:
            bit4 = bit5 = 1;
            _a = !( sys->interface->getInputState(port, _Input::IdSportsPad, _Input::A) & 1 );
            _b = !( sys->interface->getInputState(port, _Input::IdSportsPad, _Input::B) & 1 );
            nibble = _a | (_b << 1);
            break;
        case 1:
            bit4 = bit5 = 0;
            _x = sportspad[port].posX;
            nibble = _x>>4;
            break;
        case 2:
            bit4 = 1;
            bit5 = 0;
            _x = sportspad[port].posX;
            nibble = _x;
            break;
        case 3:
            bit4 = bit5 = 0;
            _y = sportspad[port].posY;
            nibble = _y>>4;
            break;
        case 4:
            bit4 = 1;
            bit5 = 0;
            _y = sportspad[port].posY;
            nibble = _y;
            break;
    }

    if (iobjectId == _Input::A) {
        return bit4;
    } if (iobjectId == _Input::B) {
        return bit5;
    } if (iobjectId == _Input::Up) {
        return (nibble >> 0) & 1;
    } if (iobjectId == _Input::Down) {
        return (nibble >> 1) & 1;
    } if (iobjectId == _Input::Left) {
        return (nibble >> 2) & 1;
    } if (iobjectId == _Input::Right) {
        return (nibble >> 3) & 1;
    }
}

void _Input::tickGuns() {
    if (smsport[Port1].deviceId != IdLightgun && smsport[Port2].deviceId != IdLightgun) {
        return;
    }

    if (cpu.getDisplayHPos() == 316) resetLatch();
    if ( smsport[Port1].deviceId == IdLightgun && !lightphaser[Port1].offscreen) {
        if (
            ((lightphaser[Port1].x + 24 ) <= cpu.getDisplayHPos()) && ((lightphaser[Port1].x + 28 ) >= cpu.getDisplayHPos())
         && ((lightphaser[Port1].y - 5) <= cpu.getDisplayVPos()) && ((lightphaser[Port1].y + 5) >= cpu.getDisplayVPos())
        ) {
            cpu.setTH(Port1, 0);
        }
    }
    if ( smsport[Port2].deviceId == IdLightgun && !lightphaser[Port2].offscreen) {
        if (
            ((lightphaser[Port2].x + 24 ) <= cpu.getDisplayHPos()) && ((lightphaser[Port2].x + 28 ) >= cpu.getDisplayHPos())
         && ((lightphaser[Port2].y - 5) <= cpu.getDisplayVPos()) && ((lightphaser[Port2].y + 5) >= cpu.getDisplayVPos())
        ) {
            cpu.setTH(Port2, 0);
        }
    }
}

void _Input::resetLatch() {
    cpu.setTH(Port1, 1);
    cpu.setTH(Port2, 1);
}

void _Input::update() {
    for (u8 port=0; port<=1; port++) {
        SmsPort& p = smsport[port];
        switch (p.deviceId) {
            case IdLightgun: {
                i16 _x = sys->interface->getInputState((_Input::Port)port, _Input::IdLightgun, _Input::X);
                i16 _y = sys->interface->getInputState((_Input::Port)port, _Input::IdLightgun, _Input::Y);

                _x += lightphaser[port].x;
                _y += lightphaser[port].y;

                _x = _max(-16, _min(284 + 16, _x));
                _y = _max(-16, _min(vdp.getDisplayHeight() + 16, _y));

                lightphaser[(_Input::Port)port].x = _x;
                lightphaser[(_Input::Port)port].y = _y;

                lightphaser[(_Input::Port)port].offscreen = false;
                if(_y < 0 || _y >= vdp.getDisplayHeight() || _x < 0 || _x >= 284) {
                    lightphaser[(_Input::Port)port].offscreen = true;
                }
            } break;
            case IdTerebi: {
                i16 _x = sys->interface->getInputState((_Input::Port)port, _Input::IdTerebi, _Input::X);
                i16 _y = sys->interface->getInputState((_Input::Port)port, _Input::IdTerebi, _Input::Y);

                _x += terebi.x;
                _y += terebi.y;

                _x = _max(0, _min(252, _x));
                _y = _max(0, _min(vdp.getDisplayHeight(), _y));

                terebi.x = (u8)_x;
                terebi.y = (u8)_y;
            } break;
            case IdPaddle: {
                i16 _x = sys->interface->getInputState((_Input::Port)port, _Input::IdPaddle, _Input::X);
                _x += paddle[port].pos;
                paddle[port].pos = _max(0, _min(255, _x));
            } break;
            case IdSportsPad: {
                i16 _x = sys->interface->getInputState((_Input::Port)port, _Input::IdSportsPad, _Input::X);
                i16 _y = sys->interface->getInputState((_Input::Port)port, _Input::IdSportsPad, _Input::Y);

                if (sys->_region() != _System::JAPAN) {
                    sportspad[port].posX = updateCo(sportspad[port].posX, -_x);
                    sportspad[port].posY = updateCo(sportspad[port].posY, -_y);
                } else {
                    sportspad[port].posX = ( sportspad[port].posX + _x ) & 0xff;
                    sportspad[port].posY = ( sportspad[port].posY + _y ) & 0xff;
                }

            } break;
        }
    }
}

i8 _Input::updateCo(i8 cur, int move) {
    //this is from meka and slightly modified
    int vel;
    int limit = 50;

    vel = cur + (move / 2);

    if (vel > limit) vel = limit;
    else if (vel < -limit) vel = -limit;

    if (vel > 0) vel -= 1;
    if (vel < 0) vel += 1;

    if (vel >= 15) vel -= 5;
    else if (vel <= -15) vel += 5;

    return vel;
}

void _Input::drawCursorIfNeeded() {
    for (u8 port=0; port<=1; port++) {
        SmsPort& p = smsport[port];
        if (p.deviceId == IdLightgun) {
            draw_cursor(port == _Input::Port1 ? 50 : 40, lightphaser[(_Input::Port)port].x, lightphaser[(_Input::Port)port].y);
        }
    }
}

void _Input::setTerebiDir(u8 val) {
    terebi.useX = (val & 1) == 1;
}

u8 _Input::getTerebiState() {
    return !( sys->interface->getInputState(Port1, IdTerebi, _Input::Trigger) & 1 );
}

u8 _Input::getTerebiPos() {
    return terebi.useX ? (u8)terebi.x : (u8)terebi.y;
}

const u8 _Input::cursor[15 * 15] = {
    0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,
    0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,
    0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,
    0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,
    0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,
    0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,
    1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,
    1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,
    0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,
    0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,
    0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,
    0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,
    0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,
    0,0,0,0,0,0,1,2,1,0,0,0,0,0,0,
};

void _Input::draw_cursor(u16 color, int x, int y) {
    u16 pixelcolor;
    for(int cy = 0; cy < 15; cy++) {
        int vy = y + cy - 7;
        if(vy <= 0 || vy >= vdp.getDisplayHeight()) continue;

        for(int cx = 0; cx < 15; cx++) {
            int vx = x + cx - 7;
            if(vx < 0 || vx >= 284) continue;
            u8 pixel = cursor[cy * 15 + cx];
            if(pixel == 0) continue;
            pixelcolor = (pixel == 1) ? 0 : color;

            *(vdp.output() + vy * 284 + vx) = pixelcolor;
        }
    }
}

void _Input::power() {
    lightphaser[_Input::Port1].offscreen = false;
    lightphaser[_Input::Port2].offscreen = false;
    lightphaser[_Input::Port1].x = 284 / 2;
    lightphaser[_Input::Port2].x = 284 / 2;
    terebi.x = 284 / 2;
    lightphaser[_Input::Port1].y = vdp.getDisplayHeight() / 2;
    lightphaser[_Input::Port2].y = vdp.getDisplayHeight() / 2;
    terebi.y = vdp.getDisplayHeight() / 2;
    terebi.useX = true;
    paddle[Port1].flip = false;
    paddle[Port2].flip = false;
    paddle[Port1].pos = 128;
    paddle[Port2].pos = 128;
    paddle[Port1].counter = 0;
    paddle[Port2].counter = 30;
    sportspad[Port1].posX = 0;
    sportspad[Port2].posX = 0;
    sportspad[Port1].posY = 0;
    sportspad[Port2].posY = 0;
    sportspad[Port1].strobeNibble = 0;
    sportspad[Port2].strobeNibble = 0;
    sportspad[Port1].clock = 0;
    sportspad[Port2].clock = 0;
    sportspad[Port1].latch = sportspad[Port2].latch = 1;
}

_Input::_Input(_System* sys, Cpu& cpu, Vdp& vdp)
:
sys(sys),
cpu(cpu),
vdp(vdp)
{
    smsport[_Input::Port1].deviceId = IdNone;
    smsport[_Input::Port2].deviceId = IdNone;
    useCustomInput = false;
}
