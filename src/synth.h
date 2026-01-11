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
    float sub_osc_mix;      // Sub-oscillator mix (0=none, 1=full)

    // Pulse Width Modulation
    float pulse_width;      // Base pulse width (0.05-0.95)
    float pwm_rate;         // PWM LFO rate (Hz)
    float pwm_depth;        // PWM LFO depth (0.0-0.45)

    // Unison (supersaw)
    int unison_count;       // 1-7 voices
    float unison_spread;    // Detune spread in cents (0-100)

    float filter_cutoff;
    float filter_resonance;
    FilterType filter_type;

    // ADSR settings (amplitude envelope)
    float attack;
    float decay;
    float sustain;
    float release;

    // Filter envelope
    float filter_env_attack;
    float filter_env_decay;
    float filter_env_sustain;
    float filter_env_release;
    float filter_env_amount;    // -1.0 to +1.0

    // LFO
    float lfo_rate;             // Hz (0.1 - 20.0)
    float lfo_depth;            // 0.0 - 1.0
    LFOWaveType lfo_type;

    // Master volume
    float volume;
} Synth;

void synth_init(Synth *s);
void synth_note_on(Synth *s, int note, int velocity);
void synth_note_off(Synth *s, int note);
void synth_panic(Synth *s);  // All notes off
float synth_process(Synth *s);

// Parameter setters
void synth_set_wave_type(Synth *s, WaveType type);
void synth_set_wave_type2(Synth *s, WaveType type);
void synth_set_osc_mix(Synth *s, float mix);
void synth_set_osc2_detune(Synth *s, float cents);
void synth_set_sub_osc_mix(Synth *s, float mix);
void synth_set_pulse_width(Synth *s, float width);
void synth_set_pwm_rate(Synth *s, float rate);
void synth_set_pwm_depth(Synth *s, float depth);
void synth_set_unison_count(Synth *s, int count);
void synth_set_unison_spread(Synth *s, float spread);
void synth_set_filter(Synth *s, float cutoff, float resonance, FilterType type);
void synth_set_adsr(Synth *s, float a, float d, float s_level, float r);
void synth_set_filter_env_adsr(Synth *s, float a, float d, float sus, float r);
void synth_set_filter_env_amount(Synth *s, float amount);
void synth_set_lfo_rate(Synth *s, float rate);
void synth_set_lfo_depth(Synth *s, float depth);
void synth_set_lfo_type(Synth *s, LFOWaveType type);
void synth_set_volume(Synth *s, float vol);

#endif // SYNTH_H
