#ifndef VOICE_H
#define VOICE_H

#include "oscillator.h"
#include "envelope.h"
#include "filter.h"
#include "lfo.h"

#define MAX_UNISON 7    // Maximum unison voices (including main)

typedef struct {
    Oscillator osc;
    Oscillator osc2;    // Second oscillator for mixing
    Oscillator sub_osc; // Sub-oscillator (octave down)

    // Unison oscillators (for supersaw effect)
    Oscillator unison_oscs[MAX_UNISON - 1];  // Extra unison oscillators
    int unison_count;       // 1-7 (1 = no unison, 7 = full supersaw)
    float unison_spread;    // Detune spread in cents (0-100)
    float osc_mix;      // 0.0 = osc1 only, 1.0 = osc2 only, 0.5 = equal mix
    float osc2_detune;  // Detune in cents (-100 to +100)
    float sub_osc_mix;  // 0.0 = no sub, 1.0 = full sub

    // Pulse Width Modulation
    float pulse_width;      // Base pulse width (0.05-0.95)
    LFO pwm_lfo;            // LFO for pulse width modulation

    Envelope env;
    Envelope filter_env;        // Filter envelope
    float filter_env_amount;    // Filter env depth (-1.0 to +1.0)
    LFO filter_lfo;             // LFO for filter modulation
    SVFilter filter;
    float base_filter_cutoff;   // Original cutoff before modulation

    int note;           // MIDI note number (-1 = inactive)
    int velocity;       // MIDI velocity (0-127)
    unsigned int age;   // for voice stealing (older = higher)
} Voice;

void voice_init(Voice *v);
void voice_note_on(Voice *v, int note, int velocity);
void voice_note_off(Voice *v);
float voice_process(Voice *v);
int voice_is_active(Voice *v);

#endif // VOICE_H
