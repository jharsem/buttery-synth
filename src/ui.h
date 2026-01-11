#ifndef UI_H
#define UI_H

#include "synth.h"
#include "effects.h"
#include "raylib.h"
#include <stdbool.h>

// Screen dimensions (landscape mode)
#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 400

typedef struct {
    // Pointers to synth and effects for parameter control
    Synth *synth;
    Effects *effects;

    // UI state
    int current_page;       // 0 = OSC, 1 = FILTER/ENV, 2 = EFFECTS
    int selected_wave;
    int selected_wave2;
    int selected_filter;

    // Waveform display buffer
    float waveform_buffer[256];
    int waveform_pos;

    // Touch state
    int active_control;     // -1 = none
    float drag_start_value;
    float last_touch_x;     // Last known touch X position
    float last_touch_y;     // Last known touch Y position
    bool was_touching;      // Was touching last frame
} UI;

void ui_init(UI *ui, Synth *synth, Effects *effects);
void ui_update(UI *ui);
void ui_draw(UI *ui);

// Add sample to waveform display
void ui_add_sample(UI *ui, float sample);

#endif // UI_H
