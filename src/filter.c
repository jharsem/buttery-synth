#include "filter.h"
#include "oscillator.h"  // for SAMPLE_RATE
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void filter_init(SVFilter *f) {
    f->low = 0.0f;
    f->high = 0.0f;
    f->band = 0.0f;
    f->notch = 0.0f;
    f->cutoff = 0.5f;
    f->resonance = 0.0f;
    f->type = FILTER_LOWPASS;
}

void filter_set_cutoff(SVFilter *f, float cutoff) {
    // Clamp to valid range
    if (cutoff < 0.0f) cutoff = 0.0f;
    if (cutoff > 1.0f) cutoff = 1.0f;
    f->cutoff = cutoff;
}

void filter_set_resonance(SVFilter *f, float resonance) {
    // Clamp to valid range
    if (resonance < 0.0f) resonance = 0.0f;
    if (resonance > 0.99f) resonance = 0.99f;  // prevent self-oscillation blowup
    f->resonance = resonance;
}

void filter_set_type(SVFilter *f, FilterType type) {
    f->type = type;
}

float filter_process(SVFilter *f, float input) {
    // Convert normalized cutoff to frequency coefficient
    // Map 0-1 to roughly 20Hz - 20kHz (exponential)
    float freq = 20.0f * powf(1000.0f, f->cutoff);
    float fc = 2.0f * sinf(M_PI * freq / SAMPLE_RATE);

    // Limit fc to prevent instability
    if (fc > 0.9f) fc = 0.9f;

    // Q factor from resonance (higher resonance = higher Q)
    float q = 1.0f - f->resonance;
    if (q < 0.05f) q = 0.05f;

    // State variable filter algorithm (Chamberlin)
    f->low = f->low + fc * f->band;
    f->high = input - f->low - q * f->band;
    f->band = fc * f->high + f->band;
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
