//ported from bsnes

#ifndef RESAMPLER_H
#define RESAMPLER_H

#include "dataTypes.h"


class Resampler
{
    struct Settings {
        unsigned channels;
        unsigned precision;
        float frequency;
        float resampledFrequency;
        float volume;

        float intensity;
        float intensityInverse;
    } settings;

    struct Buffer {
        double** sample;
        u16 rdoffset;
        u16 wroffset;
        unsigned channels;

        void setChannels(unsigned channels);

        inline double& read(unsigned channel, signed offset = 0) {
            return sample[channel][(u16)(rdoffset + offset)];
        }

        inline double& write(unsigned channel, signed offset = 0) {
            return sample[channel][(u16)(wroffset + offset)];
        }

        void clear();

        Buffer() { channels = 0; }
        ~Buffer() { setChannels(0); }
    };

    Buffer buffer;
    Buffer output;

    //resampler
    float fraction;
    float step;

    void setRatio();
    void adjustVolume();
    void hermiteSampling();
    void linearSampling();
    void nearestSampling();
    void write(float channel[]);

public:
    Resampler();
    void setChannels(unsigned channels);
    void setPrecision(unsigned precision);
    void setFrequency(float frequency, float resampledFrequency);
    void setVolume(float volume);

    void sample(signed channel[]);
    bool pending();
    void read(signed channel[]);
    void clear();
};

#endif // RESAMPLER_H
