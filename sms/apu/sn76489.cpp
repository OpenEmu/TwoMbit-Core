
#include "sn76489.h"
#include "../bus/bus.h"
#include "../cpu/cpu.h"
#include "../system/system.h"
#include "../system/interface.h"
#include "Blip_Buffer.h"

SN76489* SN76489::instance1 = 0;
SN76489* SN76489::instance2 = 0;
void SN76489::Enter1() { instance1->enter(); }
void SN76489::Enter2() { instance2->enter(); }

const int SN76489::PSGVolume[16] = {
    64, 50, 39, 31, 24, 19, 15, 12, 9, 7, 5, 4, 3, 2, 1, 0
    //1516,1205,957,760,603,479,381,303,240,191,152,120,96,76,60,0
};

u8 SN76489::mmio_read(u8 pos) {
	return cpu.openBus();
}

void SN76489::mmio_write(u8 pos, u8 value) {
    cpu.syncSn();
    ggDirection = value;
}

SN76489::SN76489(_System* sys, Cpu& cpu, Bus& bus)
: sys(sys), cpu(cpu), bus(bus)
{
    bufLeft = new Blip_Buffer();
    bufRight = new Blip_Buffer();

    bus.mmio.map(14, this);
    synthLeft = new Blip_Synth<blip_high_quality, 64 * 2>();
    synthRight = new Blip_Synth<blip_high_quality, 64 * 2>();
    synthLeft->output( bufLeft );
    synthRight->output( bufRight );
    synthLeft->volume(1.0 * ( 0.85 / 8 ));
    synthRight->volume(1.0 * ( 0.85 / 8 ));
    synthLeft->treble_eq( -8.0 );
    synthRight->treble_eq( -8.0 );

	freq[0] = &registers[0];
	freq[1] = &registers[1];
	freq[2] = &registers[2];
	freq[3] = &registers[3];

	vol[0] = &registers[4];
	vol[1] = &registers[5];
	vol[2] = &registers[6];
	vol[3] = &registers[7];
}

void SN76489::enter() {
    while (true) { //need 48 Master Cycles for a complete calculation
        runNoise(3);
        runTone(0);
        runTone(1);
        runTone(2);

        if (!isGGMode) writeSampleMono();
        else writeSampleStereo();

        add_cycles( 16 );
    }
}

void SN76489::writeSampleMono() {
    blip_sample_t sample [1];
    bufLeft->end_frame( 16 );

    if (bufLeft->read_samples( sample, 1) == 1) {
#ifdef SAMPLE_COUNTER
        sampleCounter();
#endif
        sys->interface->audioSample(sample[0], sample[0], sys->getSystemId() == 1 ? 0 : 2);
    }
}

void SN76489::writeSampleStereo() {
    bufLeft->end_frame( 16 );
    bufRight->end_frame( 16 );

    if ( bufLeft->samples_avail() == 0 || bufRight->samples_avail() == 0 ) return;

    blip_sample_t sampleLeft [1];
    blip_sample_t sampleRight [1];

    bufLeft->read_samples( sampleLeft, 1);
    bufRight->read_samples( sampleRight, 1);

#ifdef SAMPLE_COUNTER
        sampleCounter();
#endif
    sys->interface->audioSample(sampleLeft[0], sampleRight[0], sys->getSystemId() == 1 ? 0 : 2);
}

void SN76489::sampleCounter() {
	samples_count++;

	time(&curr_t);
	if(curr_t != prev_t) {
		samples_executed = samples_count;
		samples_count = 0;
	}
	prev_t = curr_t;
}

void SN76489::updateTone(u8 i) {
    int delta = (*vol[i] * (phase[i] ? 1 : -1) ) - usedAmp[i];
    if (delta != 0) {
        usedAmp[i] += delta;
        writeToBlip( i, delta );
    }
}

void SN76489::runTone(u8 i) {
    if (--counter[i] <= 0) {
        counter[i] = *freq[i];

        if (*freq[i] > 10) {
            phase[i] ^= 1;
            updateTone(i);
        } else if (*freq[i] == 1 || *freq[i] == 0 ) { //sample playback
            phase[i] = true;
            updateTone(i);
        } else { //ignored
            phase[i] ^= 1;
        }
	}
}

void SN76489::runNoise(u8 i) {
	int curamp = (NoiseShiftRegister & 0x1) * *vol[i];

	int _NoiseFreq = NoiseFreq;
	if (_NoiseFreq == 0x80) {
		_NoiseFreq = *freq[2];
		counter[i] = counter[2];
	}

	if (--counter[i] <= 0) {
		counter[i] = _NoiseFreq;
        phase[i] ^= 1;
        if ( phase[i] ) {
			int Feedback = NoiseShiftRegister;
			if ( registers[3] & 0x4 ) { //white noise
				Feedback = ((Feedback & FB_SEGAVDP) && ((Feedback & FB_SEGAVDP) ^ FB_SEGAVDP));
			} else { //periodic noise
				Feedback &= 1;
			}
			NoiseShiftRegister = (NoiseShiftRegister >> 1) | (Feedback << (SRW_SEGAVDP - 1));
			int newamp = (NoiseShiftRegister & 0x1) * *vol[i];
			writeToBlip( i, newamp - curamp );
		}
	}
}

void SN76489::writeToBlip( u8 channel, int amp ) {
    if (!isGGMode) {
        synthLeft->offset_inline( 16, amp );
        return;
    }
    int ampLeft = 0, ampRight = 0;

    if ( ( (ggDirection >> channel) & 1 ) == 1 ) { //right
        ampRight = amp;
    }
    if ( ( ( ggDirection >> (4+channel) ) & 1 ) == 1 ) { //left
        ampLeft = amp;
    }

    synthRight->offset_inline( 16, ampRight );
    synthLeft->offset_inline( 16, ampLeft );
}

void SN76489::add_cycles(u8 cycles) {
    chipClock += cycles * frequency;
	syncCpu();
}

void SN76489::syncCpu() {
    if(getClock() >= 0) {
        co_switch(cpu.getThreadHandle());
    }
}

void SN76489::initBlip() {
    if ( NULL != bufLeft->set_sample_rate( 44100 ) ) {}
    bufLeft->clock_rate( sys->_isNTSC() ? 3579545 : 3546893 );
    bufLeft->bass_freq( 180 );
    bufLeft->clear();
    if ( NULL != bufRight->set_sample_rate( 44100 ) ) {}
    bufRight->clock_rate( sys->_isNTSC() ? 3579545 : 3546893 );
    bufRight->bass_freq( 180 );
    bufRight->clear();
}

void SN76489::power() {
    if (sys->getSystemId() == 1 ) {
        create(Enter1, 3);
        instance1 = this;
    } else if (sys->getSystemId() == 2 ) {
        create(Enter2, 3);
        instance2 = this;
    }

	latched = 0;
	NoiseShiftRegister = NoiseInitialState;
	NoiseFreq =  0x10;
	samples_executed = samples_count = 0;

	for(u8 i = 0; i <= 3; i++) {
		registers[i] = 0;
		registers[4 + i] = 0;

		counter[i] = 0;
        phase[i] = true;
        usedAmp[i] = 0;
	}
	isGGMode = sys->isGGMode();
	ggDirection = 0xff;
	initBlip();
}

void SN76489::write(u8 data) {
	if (data & 0x80) {
		latched = (data >> 4) & 0x07;
	}

	switch (latched) {
		case 0:
		case 2:
		case 4: /* tone period */
			if (data & 0x80) {
				registers[latched >> 1] = (registers[latched >> 1] & 0x3f0) | (data & 0xf);
			} else {
				registers[latched >> 1] = (registers[latched >> 1] & 0x00f) | ((data & 0x3f) << 4);
            }
			break;
		case 1:
		case 3:
		case 5:
			registers[ 4 + (latched >> 1) ] = PSGVolume[ data & 0x0f ];
			break;
		case 7: /* vol */
            registers[ 4 + (latched >> 1) ] = PSGVolume[ data & 0x0f ] << 0;
			break;
		case 6: /* noise period */
			registers[3] = data & 0x0f;
			NoiseFreq = 0x10 << ( data & 0x3);
			NoiseShiftRegister = NoiseInitialState;
			break;
	}
}
