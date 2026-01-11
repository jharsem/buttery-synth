#ifndef FILTER_H
#define FILTER_H

typedef enum {
    FILTER_LOWPASS,
    FILTER_HIGHPASS,
    FILTER_BANDPASS
} FilterType;

typedef struct {
    float low;         // lowpass output
    float high;        // highpass output
    float band;        // bandpass output
    float notch;       // notch output

    float cutoff;      // normalized 0.0 - 1.0 (maps to 20Hz - 20kHz)
    float resonance;   // 0.0 - 1.0
    FilterType type;   // which output to use

    // Cached coefficients (avoid per-sample trig)
    float fc;          // frequency coefficient
    float q;           // resonance coefficient
} SVFilter;

void filter_init(SVFilter *f);
void filter_set_cutoff(SVFilter *f, float cutoff);
void filter_set_resonance(SVFilter *f, float resonance);
void filter_set_type(SVFilter *f, FilterType type);
float filter_process(SVFilter *f, float input);

#endif // FILTER_H
