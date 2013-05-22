#ifndef YM2413_H
#define YM2413_H

#include "ym2413/ym2413Core.h"
#include "../processor.h"
#include "../mem.h"

class _System;
class Bus;
class Cpu;
class Serializer;

class YM2413 : public Mmio, public Processor, public YM2413CORE {
    Cpu& cpu;
    Bus& bus;
    _System* sys;

    static YM2413* instance;
    u8 _address;

    void add_cycles(u8 cycles);
    void syncCpu();

public:
    YM2413(_System* sys, Cpu& cpu, Bus& bus);
    ~YM2413() {}
    u8 mmio_read(u8 pos);
    void mmio_write(u8 pos, u8 value);
    void power();
    void enter();
    static void Enter();
    void serialize(Serializer& s);
};

#endif // YM2413_H
