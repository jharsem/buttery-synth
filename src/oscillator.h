#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include "wavetable.h"

#define SAMPLE_RATE 44100.0f

typedef enum {
    WAVE_SINE,
    WAVE_SQUARE,
    WAVE_SAW,
    WAVE_TRIANGLE,
    WAVE_NOISE,
    WAVE_WAVETABLE
} WaveType;

typedef struct {
    float phase;
    float frequency;
    WaveType type;
    float pulse_width;      // 0.0-1.0, default 0.5 (50% duty cycle)
    unsigned int noise_seed;

    // Wavetable
    Wavetable *wavetable;   // Pointer to current wavetable
    float wt_position;      // Position within wavetable (0.0-1.0)
} Oscillator;

void osc_init(Oscillator *osc);
void osc_set_frequency(Oscillator *osc, float freq);
void osc_set_type(Oscillator *osc, WaveType type);
void osc_set_pulse_width(Oscillator *osc, float width);
void osc_set_wavetable(Oscillator *osc, WavetableType type);
void osc_set_wt_position(Oscillator *osc, float position);
float osc_generate(Oscillator *osc);

// Utility: convert MIDI note to frequency
float midi_to_freq(int note);

#endif // OSCILLATOR_H
