#include "voice.h"

void voice_init(Voice *v) {
    osc_init(&v->osc);
    osc_init(&v->osc2);
    osc_init(&v->sub_osc);

    // Initialize unison oscillators
    for (int i = 0; i < MAX_UNISON - 1; i++) {
        osc_init(&v->unison_oscs[i]);
    }
    v->unison_count = 1;    // No unison by default
    v->unison_spread = 0.0f;

    v->osc_mix = 0.0f;      // Default to osc1 only
    v->osc2_detune = 0.0f;  // No detune by default
    v->sub_osc_mix = 0.0f;  // No sub by default
    v->pulse_width = 0.5f;  // 50% duty cycle
    lfo_init(&v->pwm_lfo);
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

    // Set up unison oscillators with spread detuning
    // Spread oscillators symmetrically around the base frequency
    if (v->unison_count > 1) {
        int extra_oscs = v->unison_count - 1;
        for (int i = 0; i < extra_oscs; i++) {
            // Calculate detune for this oscillator
            // Spread from -spread to +spread across all extra oscillators
            float detune_cents;
            if (extra_oscs == 1) {
                // With 2 total, put the extra one above
                detune_cents = v->unison_spread;
            } else {
                // Spread evenly: -spread, ..., +spread
                detune_cents = -v->unison_spread + (2.0f * v->unison_spread * i / (extra_oscs - 1));
            }
            float uni_mult = powf(2.0f, detune_cents / 1200.0f);
            osc_set_frequency(&v->unison_oscs[i], freq * uni_mult);
            osc_set_type(&v->unison_oscs[i], v->osc.type);
            osc_set_pulse_width(&v->unison_oscs[i], v->pulse_width);
            // Randomize phase for each unison osc for a fuller sound
            v->unison_oscs[i].phase = (float)(i * 0.14159f);  // Spread phases
        }
    }

    // Trigger envelopes
    env_gate_on(&v->env);
    env_gate_on(&v->filter_env);

    // Reset LFO phases (key-sync)
    v->filter_lfo.phase = 0.0f;
    v->pwm_lfo.phase = 0.0f;
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

    // Apply PWM modulation to oscillators
    float pwm_mod = lfo_process(&v->pwm_lfo);
    float mod_pw = v->pulse_width + pwm_mod;
    if (mod_pw < 0.05f) mod_pw = 0.05f;
    if (mod_pw > 0.95f) mod_pw = 0.95f;
    osc_set_pulse_width(&v->osc, mod_pw);
    osc_set_pulse_width(&v->osc2, mod_pw);

    // Generate main oscillator with unison
    float osc1_out = osc_generate(&v->osc);

    // Add unison oscillators to osc1
    if (v->unison_count > 1) {
        int extra_oscs = v->unison_count - 1;
        for (int i = 0; i < extra_oscs; i++) {
            osc_set_pulse_width(&v->unison_oscs[i], mod_pw);
            osc1_out += osc_generate(&v->unison_oscs[i]);
        }
        // Gentle normalization using sqrt to preserve volume
        osc1_out /= sqrtf((float)v->unison_count);
    }

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
