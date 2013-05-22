
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "libco.h"
#include "dataTypes.h"

struct Processor {
    cothread_t thread;
    unsigned frequency;
    i32 chipClock;

    void create(void (*entrypoint_)(), unsigned frequency_) {
        if(thread) co_delete(thread);
        thread = co_create(65536 * sizeof(void*), entrypoint_);
        frequency = frequency_;
        chipClock = 0;
    }

    cothread_t getThreadHandle() {
        return thread;
    }

    unsigned getFrequency() {
        return frequency;
    }

    i32 getClock() {
        return chipClock;
    }

    i32* getClockPointer() {
        return &chipClock;
    }

    inline Processor() : thread(0) {}
};

#endif
