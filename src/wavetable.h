#ifndef WAVETABLE_H
#define WAVETABLE_H

#define WT_FRAME_SIZE 256    // Samples per frame
#define WT_NUM_FRAMES 64     // Frames per wavetable

typedef enum {
    WT_BASIC,       // Sine -> Tri -> Saw -> Square morph
    WT_PWM,         // Square with varying pulse width
    WT_HARMONICS,   // Progressive harmonic addition
    WT_FORMANT,     // Vocal-like formants
    WT_COUNT
} WavetableType;

typedef struct {
    float data[WT_NUM_FRAMES][WT_FRAME_SIZE];
    WavetableType type;
} Wavetable;

// Initialize all wavetables (call once at startup)
void wavetables_init(void);

// Get pointer to a wavetable
Wavetable* wavetable_get(WavetableType type);

// Sample a wavetable with position (0-1) and phase (0-1)
// Position selects frame (with interpolation), phase selects sample
float wavetable_sample(Wavetable *wt, float position, float phase);

// Get wavetable name for UI
const char* wavetable_name(WavetableType type);

#endif // WAVETABLE_H
