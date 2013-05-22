#include "resampler.h"
#include "helper.h"

//ported from bsnes
Resampler::Resampler()
{
    setChannels(2);
    setPrecision(16);
    setFrequency(44100.0, 44100.0);
    setVolume(1.0);
    clear();
}

void Resampler::Buffer::setChannels(unsigned channels) {
    for(unsigned c = 0; c < this->channels; c++) {
        if(sample[c]) delete[] sample[c];
    }
    //if(sample) delete[] sample;

    this->channels = channels;
    if(channels == 0) return;

    sample = new double*[channels];
    for(unsigned c = 0; c < channels; c++) {
        sample[c] = new double[65536]();
    }
}

void Resampler::Buffer::clear() {
    for(unsigned c = 0; c < channels; c++) {
        for(unsigned n = 0; n < 65536; n++) {
            sample[c][n] = 0;
        }
    }
    rdoffset = 0;
    wroffset = 0;
}

void Resampler::setChannels(unsigned channels) {
    buffer.setChannels(channels);
    output.setChannels(channels);
    settings.channels = channels;
}

void Resampler::setPrecision(unsigned precision) {
    settings.precision = precision;
    settings.intensity = 1 << (settings.precision - 1);
    settings.intensityInverse = 1.0 / settings.intensity;
}

void Resampler::setFrequency(float frequency, float resampledFrequency) {
    settings.frequency = frequency;
    settings.resampledFrequency = resampledFrequency;

    setRatio();
}

void Resampler::setVolume(float volume) {
    settings.volume = volume;
}

void Resampler::sample(signed channel[]) {
    for(unsigned c = 0; c < settings.channels; c++) {
        buffer.write(c) = (float)channel[c] * settings.intensityInverse;
    }
    buffer.wroffset++;
    hermiteSampling();
}

bool Resampler::pending() {
    return output.rdoffset != output.wroffset;
}

void Resampler::read(signed channel[]) {
    adjustVolume();

    for(unsigned c = 0; c < settings.channels; c++) {
        channel[c] = sclamp( settings.precision, output.read(c) * settings.intensity );
    }
    output.rdoffset++;
}
void Resampler::clear() {
    buffer.clear();
    output.clear();
    fraction = 0.0;
}

void Resampler::write(float channel[]) {
    for(unsigned c = 0; c < settings.channels; c++) {
        output.write(c) = channel[c];
    }
    output.wroffset++;
}

void Resampler::adjustVolume() {
    for(unsigned c = 0; c < settings.channels; c++) {
        output.read(c) *= settings.volume;
    }
}

void Resampler::setRatio() {
    fraction = 0.0;
    step = settings.frequency / settings.resampledFrequency;
}

void Resampler::hermiteSampling() {
    while(fraction <= 1.0) {
        float channel[settings.channels];

        for(unsigned n = 0; n < settings.channels; n++) {
            float a = buffer.read(n, -3);
            float b = buffer.read(n, -2);
            float c = buffer.read(n, -1);
            float d = buffer.read(n, -0);

            const float tension = 0.0;  //-1 = low, 0 = normal, +1 = high
            const float bias = 0.0;  //-1 = left, 0 = even, +1 = right

            float mu1, mu2, mu3, m0, m1, a0, a1, a2, a3;

            mu1 = fraction;
            mu2 = mu1 * mu1;
            mu3 = mu2 * mu1;

            m0  = (b - a) * (1.0 + bias) * (1.0 - tension) / 2.0;
            m0 += (c - b) * (1.0 - bias) * (1.0 - tension) / 2.0;
            m1  = (c - b) * (1.0 + bias) * (1.0 - tension) / 2.0;
            m1 += (d - c) * (1.0 - bias) * (1.0 - tension) / 2.0;

            a0 = +2 * mu3 - 3 * mu2 + 1;
            a1 =      mu3 - 2 * mu2 + mu1;
            a2 =      mu3 -     mu2;
            a3 = -2 * mu3 + 3 * mu2;

            channel[n] = (a0 * b) + (a1 * m0) + (a2 * m1) + (a3 * c);
        }

        write(channel);
        fraction += step;
    }

    buffer.rdoffset++;
    fraction -= 1.0;
}

void Resampler::linearSampling() {
    while(fraction <= 1.0) {
        float channel[settings.channels];

        for(unsigned n = 0; n < settings.channels; n++) {
            float a = buffer.read(n, -1);
            float b = buffer.read(n, -0);

            float mu = fraction;

            channel[n] = a * (1.0 - mu) + b * mu;
        }

        write(channel);
        fraction += step;
    }

    buffer.rdoffset++;
    fraction -= 1.0;
}

void Resampler::nearestSampling() {
    while(fraction <= 1.0) {
        float channel[settings.channels];

        for(unsigned n = 0; n < settings.channels; n++) {
            float a = buffer.read(n, -1);
            float b = buffer.read(n, -0);

            float mu = fraction;

            channel[n] = mu < 0.5 ? a : b;
        }

        write(channel);
        fraction += step;
    }

    buffer.rdoffset++;
    fraction -= 1.0;
}
