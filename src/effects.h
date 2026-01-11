#ifndef EFFECTS_H
#define EFFECTS_H

#include "oscillator.h"  // for SAMPLE_RATE

// Delay buffer size (max 1 second at 44100Hz)
#define DELAY_BUFFER_SIZE 44100

// Reverb uses comb and allpass filters
#define NUM_COMB_FILTERS 4
#define NUM_ALLPASS_FILTERS 2
#define COMB_BUFFER_SIZE 4096
#define ALLPASS_BUFFER_SIZE 1024

typedef struct {
    float buffer[DELAY_BUFFER_SIZE];
    int write_pos;
    float time;      // delay time in seconds (0.0 - 1.0)
    float feedback;  // 0.0 - 0.9
    float mix;       // dry/wet 0.0 - 1.0
} Delay;

typedef struct {
    float buffer[COMB_BUFFER_SIZE];
    int size;
    int pos;
    float feedback;
} CombFilter;

typedef struct {
    float buffer[ALLPASS_BUFFER_SIZE];
    int size;
    int pos;
    float feedback;
} AllpassFilter;

typedef struct {
    CombFilter combs[NUM_COMB_FILTERS];
    AllpassFilter allpasses[NUM_ALLPASS_FILTERS];
    float mix;      // dry/wet 0.0 - 1.0
    float roomsize; // 0.0 - 1.0
} Reverb;

typedef struct {
    float drive;    // 1.0 - 10.0
    float mix;      // dry/wet 0.0 - 1.0
} Distortion;

typedef struct {
    Delay delay;
    Reverb reverb;
    Distortion distortion;
} Effects;

void effects_init(Effects *fx);
float effects_process(Effects *fx, float input);

// Individual effect controls
void delay_set_time(Delay *d, float time);
void delay_set_feedback(Delay *d, float feedback);
void delay_set_mix(Delay *d, float mix);

void reverb_set_roomsize(Reverb *r, float size);
void reverb_set_mix(Reverb *r, float mix);

void distortion_set_drive(Distortion *d, float drive);
void distortion_set_mix(Distortion *d, float mix);

#endif // EFFECTS_H
