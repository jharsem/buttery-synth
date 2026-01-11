#ifndef ENVELOPE_H
#define ENVELOPE_H

typedef enum {
    ENV_IDLE,
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE
} EnvelopeStage;

typedef struct {
    float attack;      // seconds
    float decay;       // seconds
    float sustain;     // level 0.0 - 1.0
    float release;     // seconds

    EnvelopeStage stage;
    float level;       // current level
    float rate;        // current rate of change per sample
} Envelope;

void env_init(Envelope *env);
void env_set_adsr(Envelope *env, float a, float d, float s, float r);
void env_gate_on(Envelope *env);
void env_gate_off(Envelope *env);
float env_process(Envelope *env);
int env_is_active(Envelope *env);

#endif // ENVELOPE_H
