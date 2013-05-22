
#ifndef G2G_H
#define G2G_H

#include "dataTypes.h"
#include "../processor.h"
#include "../mem.h"

class _System;
class Cpu;
class Bus;

class Gear2Gear : public Processor, public Mmio
{
    _System* sys;
    Cpu& cpu;
    Bus& bus;
    static Gear2Gear* instance1;
	static Gear2Gear* instance2;
    static i64 sysClock;

	void addClocks(u8 cycles);
	void syncCpu();

	u8 sentToReceive(u8 bit);
	bool isOutputBit(u8 bit);
	void updateReg(u8 reg, u8 pos, u8 value);

	u8 read_port_GG_0x1();
	u8 read_port_GG_0x2();
	u8 read_port_GG_0x3();
	u8 read_port_GG_0x4();
	u8 read_port_GG_0x5();

	void write_port_GG_0x1(u8 value);
	void write_port_GG_0x2(u8 value);
	void write_port_GG_0x3(u8 value);
	void write_port_GG_0x4(u8 value);
	void write_port_GG_0x5(u8 value);

    enum Baud { B4800 = 746, B2400 = 1493, B1200 = 2986, B300 = 11947 }; //bps in cpu cycles

    struct {
        u8 data;
	} ggMmio[6];

	struct {
        u16 delayS;
        u16 delayP;
        u16 speed;
        u8 sendCount;
        u8 receiveCount;
        u8 sendBuffer;
        u8 receiveBuffer;

        bool parallelNmi;
        bool serialNmi;
        bool raiseNmi;
        bool bufferFill;

	} transfer;

public:
    enum Mode { PARALLEL, SERIAL };
    Gear2Gear(_System* sys, Cpu& cpu, Bus& bus);
    void enter();
	void power();
    static void Enter1();
	static void Enter2();
    u8 mmio_read(u8 pos);
	void mmio_write(u8 pos, u8 value);
    void _receive(u8 bit, bool value, Mode _mode);
    bool isNmi();
    bool getExt(u8 bit) {
        return (bool)_getBit(ggMmio[1].data, bit);
	}
    static i64 getSysClock() {
        return sysClock;
    }
};

#endif
