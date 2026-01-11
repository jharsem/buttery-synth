#include "envelope.h"
#include "oscillator.h"  // for SAMPLE_RATE

void env_init(Envelope *env) {
    env->attack = 0.01f;
    env->decay = 0.1f;
    env->sustain = 0.7f;
    env->release = 0.3f;
    env->stage = ENV_IDLE;
    env->level = 0.0f;
    env->rate = 0.0f;
}

void env_set_adsr(Envelope *env, float a, float d, float s, float r) {
    env->attack = (a > 0.001f) ? a : 0.001f;
    env->decay = (d > 0.001f) ? d : 0.001f;
    env->sustain = s;
    env->release = (r > 0.001f) ? r : 0.001f;
}

void env_gate_on(Envelope *env) {
    env->stage = ENV_ATTACK;
    env->rate = 1.0f / (env->attack * SAMPLE_RATE);
}

void env_gate_off(Envelope *env) {
    if (env->stage != ENV_IDLE) {
        env->stage = ENV_RELEASE;
        env->rate = env->level / (env->release * SAMPLE_RATE);
    }
}

float env_process(Envelope *env) {
    switch (env->stage) {
        case ENV_IDLE:
            env->level = 0.0f;
            break;

        case ENV_ATTACK:
            env->level += env->rate;
            if (env->level >= 1.0f) {
                env->level = 1.0f;
                env->stage = ENV_DECAY;
                env->rate = (1.0f - env->sustain) / (env->decay * SAMPLE_RATE);
            }
            break;

        case ENV_DECAY:
            env->level -= env->rate;
            if (env->level <= env->sustain) {
                env->level = env->sustain;
                env->stage = ENV_SUSTAIN;
            }
            break;

        case ENV_SUSTAIN:
            env->level = env->sustain;
            break;

        case ENV_RELEASE:
            env->level -= env->rate;
            if (env->level <= 0.0f) {
                env->level = 0.0f;
                env->stage = ENV_IDLE;
            }
            break;
    }

    return env->level;
}

int env_is_active(Envelope *env) {
    return env->stage != ENV_IDLE;
}
