
#ifndef SN76489_H
#define SN76489_H

//for debugging purposes
//#define SAMPLE_COUNTER

#define NoiseInitialState 0x8000
#define FB_SEGAVDP  0x0009
#define SRW_SEGAVDP 16

#define BLIP_HIGH_QUALITY 16

#include <time.h>
#include "../processor.h"
#include "../mem.h"

class _System;
class Bus;
class Cpu;
class Serializer;

template<int quality,int range> class Blip_Synth;
class Blip_Buffer;

class SN76489 : public Processor, public Mmio {

    Cpu& cpu;
    Bus& bus;
    _System* sys;
    unsigned int registers[8];

    unsigned int* vol[4];
    unsigned int* freq[4];

    bool phase[4];
	int counter[4];
    int usedAmp[4];
	u8 latched;
	int NoiseShiftRegister;
	int NoiseFreq;
	u8 ggDirection;

    Blip_Synth<BLIP_HIGH_QUALITY, 64 * 2>* synthLeft;
    Blip_Synth<BLIP_HIGH_QUALITY, 64 * 2>* synthRight;

	static const int PSGVolume[16];

    Blip_Buffer* bufLeft;
    Blip_Buffer* bufRight;
	bool isGGMode;

	void initBlip();
	void add_cycles(u8 cycles);
	void runTone(u8 i);
    void updateTone(u8 i);
	void runNoise(u8 i);
	void sampleCounter();
	unsigned samples_executed, samples_count;
	time_t prev_t, curr_t;

	static SN76489* instance1;
	static SN76489* instance2;

	void writeSampleMono();
	void writeSampleStereo();
	void writeToBlip( u8 channel, int amp );

public:
	SN76489(_System* sys, Cpu& cpu, Bus& bus);
	~SN76489() {}
    u8 mmio_read(u8 pos);
	void mmio_write(u8 pos, u8 value);
	void write(u8 data);
	void power();
	void enter();
	static void Enter1();
	static void Enter2();

    void syncCpu();

	unsigned get_samples_executed() { return samples_executed; }
    void serialize(Serializer& s);
};

#endif
