#ifndef SYNTH_H
#define SYNTH_H

#include "voice.h"

#define NUM_VOICES 4

typedef struct {
    Voice voices[NUM_VOICES];

    // Global parameters
    WaveType wave_type;
    WaveType wave_type2;    // Second oscillator waveform
    float osc_mix;          // Oscillator mix (0=osc1, 1=osc2)
    float osc2_detune;      // Detune in cents (-100 to +100)
    float filter_cutoff;
    float filter_resonance;
    FilterType filter_type;

    // ADSR settings
    float attack;
    float decay;
    float sustain;
    float release;

    // Master volume
    float volume;
} Synth;

void synth_init(Synth *s);
void synth_note_on(Synth *s, int note, int velocity);
void synth_note_off(Synth *s, int note);
float synth_process(Synth *s);

// Parameter setters
void synth_set_wave_type(Synth *s, WaveType type);
void synth_set_wave_type2(Synth *s, WaveType type);
void synth_set_osc_mix(Synth *s, float mix);
void synth_set_osc2_detune(Synth *s, float cents);
void synth_set_filter(Synth *s, float cutoff, float resonance, FilterType type);
void synth_set_adsr(Synth *s, float a, float d, float s_level, float r);
void synth_set_volume(Synth *s, float vol);

#endif // SYNTH_H
