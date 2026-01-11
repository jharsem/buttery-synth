#include "voice.h"

void voice_init(Voice *v) {
    osc_init(&v->osc);
    osc_init(&v->osc2);
    osc_init(&v->sub_osc);
    v->osc_mix = 0.0f;      // Default to osc1 only
    v->osc2_detune = 0.0f;  // No detune by default
    v->sub_osc_mix = 0.0f;  // No sub by default
    env_init(&v->env);
    env_init(&v->filter_env);
    v->filter_env_amount = 0.0f;
    lfo_init(&v->filter_lfo);
    filter_init(&v->filter);
    v->base_filter_cutoff = 0.5f;

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

    // Sub-oscillator at octave down
    osc_set_frequency(&v->sub_osc, freq * 0.5f);

    // Trigger envelopes
    env_gate_on(&v->env);
    env_gate_on(&v->filter_env);

    // Reset LFO phase (key-sync)
    v->filter_lfo.phase = 0.0f;
}

void voice_note_off(Voice *v) {
    // Release envelopes (voice stays active until envelope completes)
    env_gate_off(&v->env);
    env_gate_off(&v->filter_env);
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
    float sub_out = osc_generate(&v->sub_osc);

    // Mix main oscillators, then add sub
    float main_mix = osc1_out * (1.0f - v->osc_mix) + osc2_out * v->osc_mix;
    float sample = main_mix * (1.0f - v->sub_osc_mix * 0.5f) + sub_out * v->sub_osc_mix * 0.5f;

    // Calculate filter modulation
    float filter_env_level = env_process(&v->filter_env);
    float filter_env_mod = filter_env_level * v->filter_env_amount;
    float lfo_mod = lfo_process(&v->filter_lfo);

    // Apply modulation to filter cutoff
    float mod_cutoff = v->base_filter_cutoff + filter_env_mod + lfo_mod;
    if (mod_cutoff < 0.0f) mod_cutoff = 0.0f;
    if (mod_cutoff > 1.0f) mod_cutoff = 1.0f;
    filter_set_cutoff(&v->filter, mod_cutoff);

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
