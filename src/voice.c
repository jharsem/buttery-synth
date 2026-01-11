#include "voice.h"

void voice_init(Voice *v) {
    osc_init(&v->osc);
    osc_init(&v->osc2);
    v->osc_mix = 0.0f;      // Default to osc1 only
    v->osc2_detune = 0.0f;  // No detune by default
    env_init(&v->env);
    filter_init(&v->filter);

    v->note = -1;
    v->velocity = 0;
    v->age = 0;
}

// Need math.h for powf
#include <math.h>

void voice_note_on(Voice *v, int note, int velocity) {
    v->note = note;
    v->velocity = velocity;
    v->age = 0;

    // Set oscillator frequency from MIDI note
    float freq = midi_to_freq(note);
    osc_set_frequency(&v->osc, freq);

    // Apply detune to osc2 (cents to frequency multiplier: 2^(cents/1200))
    float detune_mult = powf(2.0f, v->osc2_detune / 1200.0f);
    osc_set_frequency(&v->osc2, freq * detune_mult);

    // Trigger envelope
    env_gate_on(&v->env);
}

void voice_note_off(Voice *v) {
    // Release envelope (voice stays active until envelope completes)
    env_gate_off(&v->env);
    // Clear note so same note can retrigger on a different voice
    v->note = -1;
}

float voice_process(Voice *v) {
    if (!voice_is_active(v)) {
        return 0.0f;
    }

    // Generate and mix oscillator samples
    float osc1_out = osc_generate(&v->osc);
    float osc2_out = osc_generate(&v->osc2);
    float sample = osc1_out * (1.0f - v->osc_mix) + osc2_out * v->osc_mix;

    // Apply filter
    sample = filter_process(&v->filter, sample);

    // Apply envelope
    float env_level = env_process(&v->env);
    sample *= env_level;

    // Apply velocity scaling
    sample *= (float)v->velocity / 127.0f;

    // Check if envelope has finished
    if (!env_is_active(&v->env)) {
        v->note = -1;
    }

    // Increment age
    v->age++;

    return sample;
}

int voice_is_active(Voice *v) {
    return v->note >= 0 || env_is_active(&v->env);
}
