#ifndef VOICE_H
#define VOICE_H

#include "oscillator.h"
#include "envelope.h"
#include "filter.h"

typedef struct {
    Oscillator osc;
    Oscillator osc2;    // Second oscillator for mixing
    float osc_mix;      // 0.0 = osc1 only, 1.0 = osc2 only, 0.5 = equal mix
    float osc2_detune;  // Detune in cents (-100 to +100)
    Envelope env;
    SVFilter filter;

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
