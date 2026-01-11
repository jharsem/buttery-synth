#include "effects.h"
#include <math.h>
#include <string.h>

// Comb filter delay times (in samples, tuned for ~44100Hz)
static const int COMB_TUNINGS[NUM_COMB_FILTERS] = {1116, 1188, 1277, 1356};
// Allpass filter delay times
static const int ALLPASS_TUNINGS[NUM_ALLPASS_FILTERS] = {556, 441};

//------------------------------------------------------------------------------
// Delay
//------------------------------------------------------------------------------

static void delay_init(Delay *d) {
    memset(d->buffer, 0, sizeof(d->buffer));
    d->write_pos = 0;
    d->time = 0.3f;
    d->feedback = 0.4f;
    d->mix = 0.3f;
}

void delay_set_time(Delay *d, float time) {
    if (time < 0.01f) time = 0.01f;
    if (time > 1.0f) time = 1.0f;
    d->time = time;
}

void delay_set_feedback(Delay *d, float feedback) {
    if (feedback < 0.0f) feedback = 0.0f;
    if (feedback > 0.9f) feedback = 0.9f;
    d->feedback = feedback;
}

void delay_set_mix(Delay *d, float mix) {
    if (mix < 0.0f) mix = 0.0f;
    if (mix > 1.0f) mix = 1.0f;
    d->mix = mix;
}

static float delay_process(Delay *d, float input) {
    int delay_samples = (int)(d->time * SAMPLE_RATE);
    if (delay_samples >= DELAY_BUFFER_SIZE) {
        delay_samples = DELAY_BUFFER_SIZE - 1;
    }

    int read_pos = d->write_pos - delay_samples;
    if (read_pos < 0) {
        read_pos += DELAY_BUFFER_SIZE;
    }

    float delayed = d->buffer[read_pos];
    float output = input + delayed * d->feedback;

    d->buffer[d->write_pos] = output;
    d->write_pos = (d->write_pos + 1) % DELAY_BUFFER_SIZE;

    return input * (1.0f - d->mix) + delayed * d->mix;
}

//------------------------------------------------------------------------------
// Reverb (Schroeder style)
//------------------------------------------------------------------------------

static void comb_init(CombFilter *c, int size, float feedback) {
    memset(c->buffer, 0, sizeof(c->buffer));
    c->size = (size < COMB_BUFFER_SIZE) ? size : COMB_BUFFER_SIZE - 1;
    c->pos = 0;
    c->feedback = feedback;
}

static float comb_process(CombFilter *c, float input) {
    float output = c->buffer[c->pos];
    c->buffer[c->pos] = input + output * c->feedback;
    c->pos = (c->pos + 1) % c->size;
    return output;
}

static void allpass_init(AllpassFilter *a, int size, float feedback) {
    memset(a->buffer, 0, sizeof(a->buffer));
    a->size = (size < ALLPASS_BUFFER_SIZE) ? size : ALLPASS_BUFFER_SIZE - 1;
    a->pos = 0;
    a->feedback = feedback;
}

static float allpass_process(AllpassFilter *a, float input) {
    float buffered = a->buffer[a->pos];
    float output = -input + buffered;
    a->buffer[a->pos] = input + buffered * a->feedback;
    a->pos = (a->pos + 1) % a->size;
    return output;
}

static void reverb_init(Reverb *r) {
    for (int i = 0; i < NUM_COMB_FILTERS; i++) {
        comb_init(&r->combs[i], COMB_TUNINGS[i], 0.84f);
    }
    for (int i = 0; i < NUM_ALLPASS_FILTERS; i++) {
        allpass_init(&r->allpasses[i], ALLPASS_TUNINGS[i], 0.5f);
    }
    r->mix = 0.2f;
    r->roomsize = 0.5f;
}

void reverb_set_roomsize(Reverb *r, float size) {
    if (size < 0.0f) size = 0.0f;
    if (size > 1.0f) size = 1.0f;
    r->roomsize = size;

    // Adjust comb filter feedback based on room size
    float feedback = 0.7f + size * 0.28f;  // 0.7 to 0.98
    for (int i = 0; i < NUM_COMB_FILTERS; i++) {
        r->combs[i].feedback = feedback;
    }
}

void reverb_set_mix(Reverb *r, float mix) {
    if (mix < 0.0f) mix = 0.0f;
    if (mix > 1.0f) mix = 1.0f;
    r->mix = mix;
}

static float reverb_process(Reverb *r, float input) {
    // Sum of parallel comb filters
    float comb_sum = 0.0f;
    for (int i = 0; i < NUM_COMB_FILTERS; i++) {
        comb_sum += comb_process(&r->combs[i], input);
    }
    comb_sum /= NUM_COMB_FILTERS;

    // Series allpass filters
    float output = comb_sum;
    for (int i = 0; i < NUM_ALLPASS_FILTERS; i++) {
        output = allpass_process(&r->allpasses[i], output);
    }

    return input * (1.0f - r->mix) + output * r->mix;
}

//------------------------------------------------------------------------------
// Distortion (soft clip waveshaping)
//------------------------------------------------------------------------------

static void distortion_init(Distortion *d) {
    d->drive = 1.0f;
    d->mix = 0.0f;
}

void distortion_set_drive(Distortion *d, float drive) {
    if (drive < 1.0f) drive = 1.0f;
    if (drive > 10.0f) drive = 10.0f;
    d->drive = drive;
}

void distortion_set_mix(Distortion *d, float mix) {
    if (mix < 0.0f) mix = 0.0f;
    if (mix > 1.0f) mix = 1.0f;
    d->mix = mix;
}

static float distortion_process(Distortion *d, float input) {
    // Apply drive
    float driven = input * d->drive;

    // Soft clip using tanh
    float distorted = tanhf(driven);

    // Normalize output (compensate for drive)
    distorted /= tanhf(d->drive);

    return input * (1.0f - d->mix) + distorted * d->mix;
}

//------------------------------------------------------------------------------
// Effects Chain
//------------------------------------------------------------------------------

void effects_init(Effects *fx) {
    delay_init(&fx->delay);
    reverb_init(&fx->reverb);
    distortion_init(&fx->distortion);
}

float effects_process(Effects *fx, float input) {
    float signal = input;

    // Order: Distortion -> Delay -> Reverb
    signal = distortion_process(&fx->distortion, signal);
    signal = delay_process(&fx->delay, signal);
    signal = reverb_process(&fx->reverb, signal);

    return signal;
}
