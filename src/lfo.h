#ifndef LFO_H
#define LFO_H

typedef enum {
    LFO_SINE,
    LFO_TRIANGLE,
    LFO_SAW,
    LFO_SQUARE
} LFOWaveType;

typedef struct {
    float phase;        // 0.0 - 1.0
    float rate;         // Hz (0.1 - 20.0)
    float depth;        // 0.0 - 1.0 (modulation amount)
    LFOWaveType type;
} LFO;

void lfo_init(LFO *lfo);
void lfo_set_rate(LFO *lfo, float rate_hz);
void lfo_set_depth(LFO *lfo, float depth);
void lfo_set_type(LFO *lfo, LFOWaveType type);
float lfo_process(LFO *lfo);  // Returns -depth to +depth

#endif // LFO_H
