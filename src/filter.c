#include "filter.h"
#include "oscillator.h"  // for SAMPLE_RATE
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Pre-calculate filter coefficient from cutoff
static void filter_update_fc(SVFilter *f) {
    float freq = 20.0f * powf(1000.0f, f->cutoff);
    f->fc = 2.0f * sinf(M_PI * freq / SAMPLE_RATE);
    if (f->fc > 0.9f) f->fc = 0.9f;
}

// Pre-calculate Q from resonance
static void filter_update_q(SVFilter *f) {
    f->q = 1.0f - f->resonance;
    if (f->q < 0.05f) f->q = 0.05f;
}

void filter_init(SVFilter *f) {
    f->low = 0.0f;
    f->high = 0.0f;
    f->band = 0.0f;
    f->notch = 0.0f;
    f->cutoff = 0.5f;
    f->resonance = 0.0f;
    f->type = FILTER_LOWPASS;
    filter_update_fc(f);
    filter_update_q(f);
}

void filter_set_cutoff(SVFilter *f, float cutoff) {
    // Clamp to valid range
    if (cutoff < 0.0f) cutoff = 0.0f;
    if (cutoff > 1.0f) cutoff = 1.0f;
    f->cutoff = cutoff;
    filter_update_fc(f);
}

void filter_set_resonance(SVFilter *f, float resonance) {
    // Clamp to valid range
    if (resonance < 0.0f) resonance = 0.0f;
    if (resonance > 0.99f) resonance = 0.99f;  // prevent self-oscillation blowup
    f->resonance = resonance;
    filter_update_q(f);
}

void filter_set_type(SVFilter *f, FilterType type) {
    f->type = type;
}

float filter_process(SVFilter *f, float input) {
    // Use cached coefficients (fc and q pre-calculated in setters)
    // State variable filter algorithm (Chamberlin)
    f->low = f->low + f->fc * f->band;
    f->high = input - f->low - f->q * f->band;
    f->band = f->fc * f->high + f->band;
    f->notch = f->high + f->low;

    // Return selected output
    switch (f->type) {
        case FILTER_LOWPASS:
            return f->low;
        case FILTER_HIGHPASS:
            return f->high;
        case FILTER_BANDPASS:
            return f->band;
        default:
            return f->low;
    }
}
