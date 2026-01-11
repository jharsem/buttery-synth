#include "wavetable.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static Wavetable wavetables[WT_COUNT];
static int initialized = 0;

static const char *wt_names[] = {
    "Basic",
    "PWM",
    "Harm",
    "Formant"
};

// Generate a single frame with specified waveform mix
static void generate_basic_frame(float *frame, float sine_amt, float tri_amt, float saw_amt, float sqr_amt) {
    for (int i = 0; i < WT_FRAME_SIZE; i++) {
        float phase = (float)i / WT_FRAME_SIZE;
        float sample = 0.0f;

        // Sine
        if (sine_amt > 0.0f) {
            sample += sine_amt * sinf(2.0f * M_PI * phase);
        }

        // Triangle
        if (tri_amt > 0.0f) {
            float tri = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
            sample += tri_amt * tri;
        }

        // Saw
        if (saw_amt > 0.0f) {
            sample += saw_amt * (2.0f * phase - 1.0f);
        }

        // Square
        if (sqr_amt > 0.0f) {
            sample += sqr_amt * ((phase < 0.5f) ? 1.0f : -1.0f);
        }

        frame[i] = sample;
    }
}

// Generate PWM frame with specified pulse width
static void generate_pwm_frame(float *frame, float pulse_width) {
    for (int i = 0; i < WT_FRAME_SIZE; i++) {
        float phase = (float)i / WT_FRAME_SIZE;
        frame[i] = (phase < pulse_width) ? 1.0f : -1.0f;
    }
}

// Generate frame with N harmonics
static void generate_harmonic_frame(float *frame, int num_harmonics) {
    for (int i = 0; i < WT_FRAME_SIZE; i++) {
        frame[i] = 0.0f;
    }

    for (int h = 1; h <= num_harmonics; h++) {
        float amp = 1.0f / h;  // Saw-like harmonic rolloff
        for (int i = 0; i < WT_FRAME_SIZE; i++) {
            float phase = (float)i / WT_FRAME_SIZE;
            frame[i] += amp * sinf(2.0f * M_PI * phase * h);
        }
    }

    // Normalize
    float max_val = 0.0f;
    for (int i = 0; i < WT_FRAME_SIZE; i++) {
        if (fabsf(frame[i]) > max_val) max_val = fabsf(frame[i]);
    }
    if (max_val > 0.0f) {
        for (int i = 0; i < WT_FRAME_SIZE; i++) {
            frame[i] /= max_val;
        }
    }
}

// Generate formant frame with resonant peaks
static void generate_formant_frame(float *frame, float formant_freq) {
    // Simple formant: resonant peak simulation
    for (int i = 0; i < WT_FRAME_SIZE; i++) {
        float phase = (float)i / WT_FRAME_SIZE;
        float sample = 0.0f;

        // Base sawtooth
        sample = 2.0f * phase - 1.0f;

        // Add formant resonance (multiple of fundamental)
        sample += 0.5f * sinf(2.0f * M_PI * phase * formant_freq);
        sample += 0.25f * sinf(2.0f * M_PI * phase * formant_freq * 1.5f);

        frame[i] = sample * 0.5f;  // Scale down
    }

    // Normalize
    float max_val = 0.0f;
    for (int i = 0; i < WT_FRAME_SIZE; i++) {
        if (fabsf(frame[i]) > max_val) max_val = fabsf(frame[i]);
    }
    if (max_val > 0.0f) {
        for (int i = 0; i < WT_FRAME_SIZE; i++) {
            frame[i] /= max_val;
        }
    }
}

void wavetables_init(void) {
    if (initialized) return;

    // WT_BASIC: Morph sine -> triangle -> saw -> square
    for (int f = 0; f < WT_NUM_FRAMES; f++) {
        float pos = (float)f / (WT_NUM_FRAMES - 1);
        float sine = 0, tri = 0, saw = 0, sqr = 0;

        if (pos < 0.333f) {
            // Sine to Triangle
            float t = pos / 0.333f;
            sine = 1.0f - t;
            tri = t;
        } else if (pos < 0.666f) {
            // Triangle to Saw
            float t = (pos - 0.333f) / 0.333f;
            tri = 1.0f - t;
            saw = t;
        } else {
            // Saw to Square
            float t = (pos - 0.666f) / 0.334f;
            saw = 1.0f - t;
            sqr = t;
        }

        generate_basic_frame(wavetables[WT_BASIC].data[f], sine, tri, saw, sqr);
    }
    wavetables[WT_BASIC].type = WT_BASIC;

    // WT_PWM: Pulse width from 5% to 95%
    for (int f = 0; f < WT_NUM_FRAMES; f++) {
        float pw = 0.05f + 0.9f * (float)f / (WT_NUM_FRAMES - 1);
        generate_pwm_frame(wavetables[WT_PWM].data[f], pw);
    }
    wavetables[WT_PWM].type = WT_PWM;

    // WT_HARMONICS: 1 to 32 harmonics
    for (int f = 0; f < WT_NUM_FRAMES; f++) {
        int harmonics = 1 + (31 * f / (WT_NUM_FRAMES - 1));
        generate_harmonic_frame(wavetables[WT_HARMONICS].data[f], harmonics);
    }
    wavetables[WT_HARMONICS].type = WT_HARMONICS;

    // WT_FORMANT: Formant sweep from low to high
    for (int f = 0; f < WT_NUM_FRAMES; f++) {
        float formant = 2.0f + 10.0f * (float)f / (WT_NUM_FRAMES - 1);
        generate_formant_frame(wavetables[WT_FORMANT].data[f], formant);
    }
    wavetables[WT_FORMANT].type = WT_FORMANT;

    initialized = 1;
}

Wavetable* wavetable_get(WavetableType type) {
    if (type >= WT_COUNT) type = WT_BASIC;
    return &wavetables[type];
}

float wavetable_sample(Wavetable *wt, float position, float phase) {
    // Clamp position
    if (position < 0.0f) position = 0.0f;
    if (position > 1.0f) position = 1.0f;

    // Wrap phase
    phase = phase - (int)phase;
    if (phase < 0.0f) phase += 1.0f;

    // Calculate frame indices for interpolation
    float frame_pos = position * (WT_NUM_FRAMES - 1);
    int frame_lo = (int)frame_pos;
    int frame_hi = frame_lo + 1;
    if (frame_hi >= WT_NUM_FRAMES) frame_hi = WT_NUM_FRAMES - 1;
    float frame_frac = frame_pos - frame_lo;

    // Calculate sample index for interpolation
    float sample_pos = phase * WT_FRAME_SIZE;
    int sample_lo = (int)sample_pos;
    int sample_hi = (sample_lo + 1) % WT_FRAME_SIZE;
    float sample_frac = sample_pos - sample_lo;

    // Bilinear interpolation (frame and sample)
    float s00 = wt->data[frame_lo][sample_lo];
    float s01 = wt->data[frame_lo][sample_hi];
    float s10 = wt->data[frame_hi][sample_lo];
    float s11 = wt->data[frame_hi][sample_hi];

    float s0 = s00 + sample_frac * (s01 - s00);
    float s1 = s10 + sample_frac * (s11 - s10);

    return s0 + frame_frac * (s1 - s0);
}

const char* wavetable_name(WavetableType type) {
    if (type >= WT_COUNT) return "???";
    return wt_names[type];
}
