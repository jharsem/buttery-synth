#include "lfo.h"
#include <math.h>

#define SAMPLE_RATE 44100.0f

void lfo_init(LFO *lfo) {
    lfo->phase = 0.0f;
    lfo->rate = 1.0f;       // 1 Hz default
    lfo->depth = 0.0f;      // Off by default
    lfo->type = LFO_SINE;
}

void lfo_set_rate(LFO *lfo, float rate_hz) {
    if (rate_hz < 0.1f) rate_hz = 0.1f;
    if (rate_hz > 20.0f) rate_hz = 20.0f;
    lfo->rate = rate_hz;
}

void lfo_set_depth(LFO *lfo, float depth) {
    if (depth < 0.0f) depth = 0.0f;
    if (depth > 1.0f) depth = 1.0f;
    lfo->depth = depth;
}

void lfo_set_type(LFO *lfo, LFOWaveType type) {
    lfo->type = type;
}

float lfo_process(LFO *lfo) {
    // Advance phase
    lfo->phase += lfo->rate / SAMPLE_RATE;
    if (lfo->phase >= 1.0f) {
        lfo->phase -= 1.0f;
    }

    // Generate waveform (bipolar -1 to +1)
    float value = 0.0f;
    switch (lfo->type) {
        case LFO_SINE:
            value = sinf(lfo->phase * 2.0f * 3.14159265f);
            break;
        case LFO_TRIANGLE:
            if (lfo->phase < 0.5f) {
                value = 4.0f * lfo->phase - 1.0f;
            } else {
                value = 3.0f - 4.0f * lfo->phase;
            }
            break;
        case LFO_SAW:
            value = 2.0f * lfo->phase - 1.0f;
            break;
        case LFO_SQUARE:
            value = (lfo->phase < 0.5f) ? 1.0f : -1.0f;
            break;
    }

    // Scale by depth
    return value * lfo->depth;
}
