#include "oscillator.h"
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void osc_init(Oscillator *osc) {
    osc->phase = 0.0f;
    osc->frequency = 440.0f;
    osc->type = WAVE_SINE;
    osc->pulse_width = 0.5f;  // 50% duty cycle
    osc->noise_seed = 12345;
}

void osc_set_frequency(Oscillator *osc, float freq) {
    osc->frequency = freq;
}

void osc_set_type(Oscillator *osc, WaveType type) {
    osc->type = type;
}

void osc_set_pulse_width(Oscillator *osc, float width) {
    if (width < 0.05f) width = 0.05f;  // Prevent silent extremes
    if (width > 0.95f) width = 0.95f;
    osc->pulse_width = width;
}

// Simple noise generator (xorshift)
static float generate_noise(unsigned int *seed) {
    *seed ^= *seed << 13;
    *seed ^= *seed >> 17;
    *seed ^= *seed << 5;
    return (float)(*seed) / (float)0xFFFFFFFF * 2.0f - 1.0f;
}

float osc_generate(Oscillator *osc) {
    float sample = 0.0f;
    float phase = osc->phase;

    switch (osc->type) {
        case WAVE_SINE:
            sample = sinf(2.0f * M_PI * phase);
            break;

        case WAVE_SQUARE:
            sample = (phase < osc->pulse_width) ? 1.0f : -1.0f;
            break;

        case WAVE_SAW:
            sample = 2.0f * phase - 1.0f;
            break;

        case WAVE_TRIANGLE:
            if (phase < 0.25f) {
                sample = 4.0f * phase;
            } else if (phase < 0.75f) {
                sample = 2.0f - 4.0f * phase;
            } else {
                sample = 4.0f * phase - 4.0f;
            }
            break;

        case WAVE_NOISE:
            sample = generate_noise(&osc->noise_seed);
            break;
    }

    // Advance phase
    float inc = osc->frequency / SAMPLE_RATE;
    osc->phase += inc;
    if (osc->phase >= 1.0f) {
        osc->phase -= 1.0f;
    }

    return sample;
}

float midi_to_freq(int note) {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}
