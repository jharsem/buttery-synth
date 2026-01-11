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
    int current_page;       // 0 = OSC, 1 = FLT, 2 = FX, 3 = MOD, 4 = PRESET
    int selected_wave;
    int selected_wave2;
    int selected_filter;
    int selected_lfo;
    int current_preset;     // 1-99
    char preset_name[32];   // Current preset name
    bool editing_name;      // True when editing preset name

    // Settings
    int buffer_size;        // 0=512, 1=256, 2=128
    bool panic_triggered;   // True when panic button pressed
    bool buffer_changed;    // True when buffer size changed (needs restart)

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
