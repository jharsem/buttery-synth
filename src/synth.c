#include "synth.h"
#include <stddef.h>

void synth_init(Synth *s) {
    for (int i = 0; i < NUM_VOICES; i++) {
        voice_init(&s->voices[i]);
    }

    s->wave_type = WAVE_SAW;
    s->wave_type2 = WAVE_SQUARE;
    s->osc_mix = 0.0f;      // Default to osc1 only
    s->osc2_detune = 0.0f;  // No detune by default
    s->filter_cutoff = 0.7f;
    s->filter_resonance = 0.2f;
    s->filter_type = FILTER_LOWPASS;

    s->attack = 0.01f;
    s->decay = 0.1f;
    s->sustain = 0.7f;
    s->release = 0.3f;

    s->volume = 0.5f;
}

// Find a free voice or steal the oldest one
static Voice* find_voice(Synth *s) {
    // First, look for an inactive voice
    for (int i = 0; i < NUM_VOICES; i++) {
        if (!voice_is_active(&s->voices[i])) {
            return &s->voices[i];
        }
    }

    // No free voice, steal the oldest
    Voice *oldest = &s->voices[0];
    for (int i = 1; i < NUM_VOICES; i++) {
        if (s->voices[i].age > oldest->age) {
            oldest = &s->voices[i];
        }
    }
    return oldest;
}

// Find voice playing a specific note (must be active and not in release)
static Voice* find_voice_by_note(Synth *s, int note) {
    for (int i = 0; i < NUM_VOICES; i++) {
        if (s->voices[i].note == note && voice_is_active(&s->voices[i])) {
            return &s->voices[i];
        }
    }
    return NULL;
}

void synth_note_on(Synth *s, int note, int velocity) {
    if (velocity == 0) {
        synth_note_off(s, note);
        return;
    }

    Voice *v = find_voice(s);

    // Apply global settings to voice
    osc_set_type(&v->osc, s->wave_type);
    osc_set_type(&v->osc2, s->wave_type2);
    v->osc_mix = s->osc_mix;
    v->osc2_detune = s->osc2_detune;
    filter_set_cutoff(&v->filter, s->filter_cutoff);
    filter_set_resonance(&v->filter, s->filter_resonance);
    filter_set_type(&v->filter, s->filter_type);
    env_set_adsr(&v->env, s->attack, s->decay, s->sustain, s->release);

    voice_note_on(v, note, velocity);
}

void synth_note_off(Synth *s, int note) {
    Voice *v = find_voice_by_note(s, note);
    if (v != NULL) {
        voice_note_off(v);
    }
}

float synth_process(Synth *s) {
    float mix = 0.0f;
    int active_count = 0;

    for (int i = 0; i < NUM_VOICES; i++) {
        if (voice_is_active(&s->voices[i])) {
            mix += voice_process(&s->voices[i]);
            active_count++;
        }
    }

    // Normalize by number of active voices to prevent clipping
    if (active_count > 0) {
        mix /= (float)active_count;
    }

    return mix * s->volume;
}

void synth_set_wave_type(Synth *s, WaveType type) {
    s->wave_type = type;
    // Update active voices
    for (int i = 0; i < NUM_VOICES; i++) {
        osc_set_type(&s->voices[i].osc, type);
    }
}

void synth_set_wave_type2(Synth *s, WaveType type) {
    s->wave_type2 = type;
    // Update active voices
    for (int i = 0; i < NUM_VOICES; i++) {
        osc_set_type(&s->voices[i].osc2, type);
    }
}

void synth_set_osc_mix(Synth *s, float mix) {
    if (mix < 0.0f) mix = 0.0f;
    if (mix > 1.0f) mix = 1.0f;
    s->osc_mix = mix;
    // Update active voices
    for (int i = 0; i < NUM_VOICES; i++) {
        s->voices[i].osc_mix = mix;
    }
}

void synth_set_osc2_detune(Synth *s, float cents) {
    if (cents < -100.0f) cents = -100.0f;
    if (cents > 100.0f) cents = 100.0f;
    s->osc2_detune = cents;
    // Update active voices
    for (int i = 0; i < NUM_VOICES; i++) {
        s->voices[i].osc2_detune = cents;
    }
}

void synth_set_filter(Synth *s, float cutoff, float resonance, FilterType type) {
    s->filter_cutoff = cutoff;
    s->filter_resonance = resonance;
    s->filter_type = type;
    // Update active voices
    for (int i = 0; i < NUM_VOICES; i++) {
        filter_set_cutoff(&s->voices[i].filter, cutoff);
        filter_set_resonance(&s->voices[i].filter, resonance);
        filter_set_type(&s->voices[i].filter, type);
    }
}

void synth_set_adsr(Synth *s, float a, float d, float s_level, float r) {
    s->attack = a;
    s->decay = d;
    s->sustain = s_level;
    s->release = r;
}

void synth_set_volume(Synth *s, float vol) {
    if (vol < 0.0f) vol = 0.0f;
    if (vol > 1.0f) vol = 1.0f;
    s->volume = vol;
}
