#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#define SAMPLE_RATE 44100.0f

typedef enum {
    WAVE_SINE,
    WAVE_SQUARE,
    WAVE_SAW,
    WAVE_TRIANGLE,
    WAVE_NOISE
} WaveType;

typedef struct {
    float phase;
    float frequency;
    WaveType type;
    float pulse_width;      // 0.0-1.0, default 0.5 (50% duty cycle)
    unsigned int noise_seed;
} Oscillator;

void osc_init(Oscillator *osc);
void osc_set_frequency(Oscillator *osc, float freq);
void osc_set_type(Oscillator *osc, WaveType type);
void osc_set_pulse_width(Oscillator *osc, float width);
float osc_generate(Oscillator *osc);

// Utility: convert MIDI note to frequency
float midi_to_freq(int note);

#endif // OSCILLATOR_H
