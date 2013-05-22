
#ifndef _INPUT_H
#define _INPUT_H

#include "dataTypes.h"

class _System;
class Cpu;
class Vdp;
class Serializer;

class _Input
{
    _System* sys;
    Cpu& cpu;
    Vdp& vdp;

    static const u8 cursor[15 * 15];
    void draw_cursor(u16 color, int x, int y);
    u8 getNibble(u16 sequence, u8 strobeNibble);

    bool useCustomInput;
public:
    enum Port { Port1 = 0, Port2 = 1, PortNone = 2 };

    enum DeviceID {
		IdNone = 0,
		IdJoypad = 1,
		IdLightgun = 2,
		IdPaddle = 3,
		IdSportsPad = 4,
        IdTerebi = 5,
        IdMisc = 6
	};

	enum IObjectID {
        Up		=	0, Down     =	1,
		Left	=	2, Right=	3,
		A		=	4, B	=	5,
		Start   =   6,
		X		=	7, Y	=	8,
        Trigger	=	9, Pause=	10
	};

    struct SmsPort {
        DeviceID deviceId;
    } smsport[2];

	struct {
        int x;
        int y;
        bool offscreen;
	} lightphaser[2];

    struct {
        u8 x;
        u8 y;
        bool useX;
    } terebi;

	struct {
        bool flip;
        u8 pos;
        unsigned counter;
    } paddle[2];

	struct {
        i8 posX;
        i8 posY;
        u8 strobeNibble;
        u16 clock;
        bool latch;
	} sportspad[2];

	u8 readPort(_Input::Port port, _Input::IObjectID iobjectId);
	u8 readPaddle( _Input::Port port, _Input::IObjectID iobjectId);
	u8 readSportsPad( _Input::Port port, _Input::IObjectID iobjectId);
    u8 readSportsPadJap(_Input::Port port,_Input::IObjectID iobjectId);
    void updateSportspad(u8 _old, u8 _new);

    void tickGuns();
    void tickJapSportspad(_Input::Port port);

	void setPortDevice(_Input::Port port, _Input::DeviceID deviceId);
    i8 updateCo(i8 cur, int move);
    void setTerebiDir(u8 val);
    u8 getTerebiState();
    u8 getTerebiPos();

	void update();
	void resetLatch();
	void power();

	void drawCursorIfNeeded();
    void updatePaddle();
    void flipPaddle(Port port) {
        paddle[port].flip ^= 1;
	}

    void setCustomInput();
    bool isCustomInput() {
        return useCustomInput;
    }
    void serialize(Serializer& s);

	_Input(_System* sys, Cpu& cpu, Vdp& vdp);
};

#endif
