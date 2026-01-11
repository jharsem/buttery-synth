#include "ui.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Physical display dimensions (must match main.c)
#define PHYSICAL_WIDTH  400
#define PHYSICAL_HEIGHT 1280

// Get touch position - per CLAUDE.md, just scale (no axis swap needed)
static Vector2 GetTransformedTouch(void) {
    Vector2 pos = GetMousePosition();

    // Per CLAUDE.md: touch panel handles rotation, just need scaling
    // Scale: x * 3.2 (400 -> 1280), y * 0.3125 (1280 -> 400)
    Vector2 transformed;
    transformed.x = pos.x * (1280.0f / 400.0f);
    transformed.y = pos.y * (400.0f / 1280.0f);
    return transformed;
}

// Layout constants
#define PANEL_Y       20
#define PANEL_HEIGHT  200
#define PANEL_WIDTH   300
#define PANEL_MARGIN  16

#define SLIDER_HEIGHT 20
#define SLIDER_WIDTH  160
#define LABEL_WIDTH   50

// Colors
#define BG_COLOR      (Color){30, 30, 40, 255}
#define PANEL_COLOR   (Color){45, 45, 60, 255}
#define SLIDER_BG     (Color){60, 60, 80, 255}
#define SLIDER_FG     (Color){100, 180, 255, 255}
#define TEXT_COLOR    (Color){220, 220, 230, 255}
#define WAVE_COLOR    (Color){80, 255, 120, 255}

// Control IDs
enum {
    CTRL_NONE = -1,
    CTRL_WAVE_TYPE,
    CTRL_WAVE_TYPE2,
    CTRL_OSC_MIX,
    CTRL_OSC2_DETUNE,
    CTRL_FILTER_TYPE,
    CTRL_FILTER_CUTOFF,
    CTRL_FILTER_RESO,
    CTRL_ATTACK,
    CTRL_DECAY,
    CTRL_SUSTAIN,
    CTRL_RELEASE,
    CTRL_DELAY_TIME,
    CTRL_DELAY_MIX,
    CTRL_REVERB_SIZE,
    CTRL_REVERB_MIX,
    CTRL_DIST_DRIVE,
    CTRL_DIST_MIX,
    CTRL_VOLUME
};

static const char *WAVE_NAMES[] = {"SIN", "SQR", "SAW", "TRI", "NSE"};
static const char *FILTER_NAMES[] = {"LP", "HP", "BP"};
static const char *PAGE_NAMES[] = {"OSC", "FLT", "FX"};

void ui_init(UI *ui, Synth *synth, Effects *effects) {
    ui->synth = synth;
    ui->effects = effects;
    ui->current_page = 0;
    ui->selected_wave = synth->wave_type;
    ui->selected_wave2 = synth->wave_type2;
    ui->selected_filter = synth->filter_type;
    ui->active_control = CTRL_NONE;
    ui->waveform_pos = 0;
    ui->last_touch_x = 0;
    ui->last_touch_y = 0;
    ui->was_touching = false;
    memset(ui->waveform_buffer, 0, sizeof(ui->waveform_buffer));
}

// Draw a horizontal slider, returns new value if changed
static float draw_slider(const char *label, float value, float min, float max,
                        int x, int y, int ctrl_id, UI *ui) {
    // Label
    DrawText(label, x, y + 2, 16, TEXT_COLOR);

    // Slider background
    Rectangle slider_rect = {x + LABEL_WIDTH, y, SLIDER_WIDTH, SLIDER_HEIGHT};
    DrawRectangleRec(slider_rect, SLIDER_BG);

    // Slider fill
    float norm = (value - min) / (max - min);
    Rectangle fill_rect = {x + LABEL_WIDTH, y, SLIDER_WIDTH * norm, SLIDER_HEIGHT};
    DrawRectangleRec(fill_rect, SLIDER_FG);

    // Value text
    char val_str[16];
    snprintf(val_str, sizeof(val_str), "%.2f", value);
    DrawText(val_str, x + LABEL_WIDTH + SLIDER_WIDTH + 5, y + 2, 16, TEXT_COLOR);

    // Touch/mouse handling
    bool pressing = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    Vector2 mouse = GetTransformedTouch();

    // Check if mouse position is valid (not at origin)
    bool valid_pos = (mouse.x > 1 || mouse.y > 1);

    if (pressing && valid_pos) {
        // Check if pressing on this slider
        if (CheckCollisionPointRec(mouse, slider_rect)) {
            ui->active_control = ctrl_id;
        }

        // If this slider is active, update value
        if (ui->active_control == ctrl_id) {
            float new_norm = (mouse.x - slider_rect.x) / slider_rect.width;
            if (new_norm < 0) new_norm = 0;
            if (new_norm > 1) new_norm = 1;
            return min + new_norm * (max - min);
        }
    } else if (!pressing) {
        // Released - clear active control
        if (ui->active_control == ctrl_id) {
            ui->active_control = CTRL_NONE;
        }
    }

    return value;
}

// Draw button row, returns selected index
static int draw_button_row(const char *label, const char **options, int count,
                          int selected, int x, int y) {
    DrawText(label, x, y + 5, 14, TEXT_COLOR);

    int btn_x = x + LABEL_WIDTH;
    int btn_width = 44;
    int btn_height = 22;

    for (int i = 0; i < count; i++) {
        Rectangle btn = {btn_x + i * (btn_width + 3), y, btn_width, btn_height};

        Color btn_color = (i == selected) ? SLIDER_FG : SLIDER_BG;
        DrawRectangleRec(btn, btn_color);

        int text_width = MeasureText(options[i], 10);
        DrawText(options[i], btn.x + (btn_width - text_width) / 2, btn.y + 6, 10,
                 (i == selected) ? BG_COLOR : TEXT_COLOR);

        // Touch handling
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetTransformedTouch();
            if (CheckCollisionPointRec(mouse, btn)) {
                return i;
            }
        }
    }

    return selected;
}

void ui_update(UI *ui) {
    // Input handling is done in ui_draw for touch controls
    (void)ui;  // Suppress unused warning
}

void ui_draw(UI *ui) {
    Synth *s = ui->synth;
    Effects *fx = ui->effects;

    ClearBackground(BG_COLOR);

    // Page tabs at top
    int tab_width = 80;
    int tab_height = 30;
    int tab_y = 5;
    for (int i = 0; i < 3; i++) {
        Rectangle tab = {PANEL_MARGIN + i * (tab_width + 5), tab_y, tab_width, tab_height};
        Color tab_color = (i == ui->current_page) ? SLIDER_FG : SLIDER_BG;
        DrawRectangleRec(tab, tab_color);
        int tw = MeasureText(PAGE_NAMES[i], 14);
        DrawText(PAGE_NAMES[i], tab.x + (tab_width - tw) / 2, tab.y + 8, 14,
                 (i == ui->current_page) ? BG_COLOR : TEXT_COLOR);

        // Touch handling for tabs
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetTransformedTouch();
            if (CheckCollisionPointRec(mouse, tab)) {
                ui->current_page = i;
            }
        }
    }

    int panel_y = tab_y + tab_height + 10;
    int panel_x = PANEL_MARGIN;
    int content_height = PANEL_HEIGHT - 10;

    // Draw page content based on current page
    if (ui->current_page == 0) {
        // OSC PAGE: OSC1 + OSC2 + Mix
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH + 20, content_height, PANEL_COLOR);
        DrawText("OSC 1", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);
        int new_wave = draw_button_row("Wave", WAVE_NAMES, 5, ui->selected_wave,
                                       panel_x + 10, panel_y + 25);
        if (new_wave != ui->selected_wave) {
            ui->selected_wave = new_wave;
            synth_set_wave_type(s, (WaveType)new_wave);
        }

        panel_x += PANEL_WIDTH + 20 + PANEL_MARGIN;
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH + 20, content_height, PANEL_COLOR);
        DrawText("OSC 2", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);
        int new_wave2 = draw_button_row("Wave", WAVE_NAMES, 5, ui->selected_wave2,
                                        panel_x + 10, panel_y + 25);
        if (new_wave2 != ui->selected_wave2) {
            ui->selected_wave2 = new_wave2;
            synth_set_wave_type2(s, (WaveType)new_wave2);
        }

        panel_x += PANEL_WIDTH + 20 + PANEL_MARGIN;
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH, content_height, PANEL_COLOR);
        DrawText("MIX", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);
        float new_mix = draw_slider("O1/O2", s->osc_mix, 0.0f, 1.0f,
                                    panel_x + 10, panel_y + 30, CTRL_OSC_MIX, ui);
        if (new_mix != s->osc_mix) {
            synth_set_osc_mix(s, new_mix);
        }
        float new_detune = draw_slider("Det", s->osc2_detune, -100.0f, 100.0f,
                                       panel_x + 10, panel_y + 60, CTRL_OSC2_DETUNE, ui);
        if (new_detune != s->osc2_detune) {
            synth_set_osc2_detune(s, new_detune);
        }
        s->volume = draw_slider("Vol", s->volume, 0.0f, 1.0f,
                                panel_x + 10, panel_y + 90, CTRL_VOLUME, ui);

    } else if (ui->current_page == 1) {
        // FILTER PAGE: Filter + Envelope
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH + 60, content_height, PANEL_COLOR);
        DrawText("FILTER", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);
        int new_filter = draw_button_row("Type", FILTER_NAMES, 3, ui->selected_filter,
                                         panel_x + 10, panel_y + 25);
        if (new_filter != ui->selected_filter) {
            ui->selected_filter = new_filter;
            s->filter_type = (FilterType)new_filter;
            synth_set_filter(s, s->filter_cutoff, s->filter_resonance, s->filter_type);
        }
        float new_cutoff = draw_slider("Cut", s->filter_cutoff, 0.0f, 1.0f,
                                       panel_x + 10, panel_y + 55, CTRL_FILTER_CUTOFF, ui);
        float new_reso = draw_slider("Res", s->filter_resonance, 0.0f, 0.95f,
                                     panel_x + 10, panel_y + 85, CTRL_FILTER_RESO, ui);
        if (new_cutoff != s->filter_cutoff || new_reso != s->filter_resonance) {
            synth_set_filter(s, new_cutoff, new_reso, s->filter_type);
        }

        panel_x += PANEL_WIDTH + 60 + PANEL_MARGIN;
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH + 60, content_height, PANEL_COLOR);
        DrawText("ENVELOPE", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);
        float new_a = draw_slider("A", s->attack, 0.001f, 2.0f,
                                  panel_x + 10, panel_y + 30, CTRL_ATTACK, ui);
        float new_d = draw_slider("D", s->decay, 0.001f, 2.0f,
                                  panel_x + 10, panel_y + 60, CTRL_DECAY, ui);
        float new_sus = draw_slider("S", s->sustain, 0.0f, 1.0f,
                                    panel_x + 10, panel_y + 90, CTRL_SUSTAIN, ui);
        float new_r = draw_slider("R", s->release, 0.001f, 3.0f,
                                  panel_x + 10, panel_y + 120, CTRL_RELEASE, ui);
        if (new_a != s->attack || new_d != s->decay ||
            new_sus != s->sustain || new_r != s->release) {
            synth_set_adsr(s, new_a, new_d, new_sus, new_r);
        }

    } else {
        // FX PAGE: Effects
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH + 40, content_height, PANEL_COLOR);
        DrawText("DELAY", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);
        fx->delay.time = draw_slider("Time", fx->delay.time, 0.01f, 1.0f,
                                     panel_x + 10, panel_y + 30, CTRL_DELAY_TIME, ui);
        fx->delay.mix = draw_slider("Mix", fx->delay.mix, 0.0f, 1.0f,
                                    panel_x + 10, panel_y + 60, CTRL_DELAY_MIX, ui);

        panel_x += PANEL_WIDTH + 40 + PANEL_MARGIN;
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH + 40, content_height, PANEL_COLOR);
        DrawText("REVERB", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);
        fx->reverb.mix = draw_slider("Mix", fx->reverb.mix, 0.0f, 1.0f,
                                     panel_x + 10, panel_y + 30, CTRL_REVERB_MIX, ui);

        panel_x += PANEL_WIDTH + 40 + PANEL_MARGIN;
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH + 40, content_height, PANEL_COLOR);
        DrawText("DISTORT", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);
        fx->distortion.mix = draw_slider("Mix", fx->distortion.mix, 0.0f, 1.0f,
                                         panel_x + 10, panel_y + 30, CTRL_DIST_MIX, ui);
        fx->distortion.drive = draw_slider("Drv", fx->distortion.drive, 1.0f, 10.0f,
                                           panel_x + 10, panel_y + 60, CTRL_DIST_DRIVE, ui);
    }

    // Waveform display (bottom area)
    int wave_y = panel_y + content_height + 15;
    int wave_height = SCREEN_HEIGHT - wave_y - 10;
    int wave_width = SCREEN_WIDTH - 40;

    DrawRectangle(20, wave_y, wave_width, wave_height, PANEL_COLOR);

    // Draw waveform
    int center_y = wave_y + wave_height / 2;
    int prev_y = center_y;

    for (int i = 0; i < 256; i++) {
        int idx = (ui->waveform_pos + i) % 256;
        int y_pos = center_y - (int)(ui->waveform_buffer[idx] * (wave_height / 2 - 5));
        int x_pos = 25 + (i * (wave_width - 10)) / 256;

        if (i > 0) {
            DrawLine(25 + ((i - 1) * (wave_width - 10)) / 256, prev_y,
                     x_pos, y_pos, WAVE_COLOR);
        }
        prev_y = y_pos;
    }

    // FPS counter
    DrawFPS(SCREEN_WIDTH - 80, 8);
}

void ui_add_sample(UI *ui, float sample) {
    // Downsample for display (only update every ~172 samples for 256 point display at 44100Hz)
    static int sample_count = 0;
    sample_count++;
    if (sample_count >= 172) {
        ui->waveform_buffer[ui->waveform_pos] = sample;
        ui->waveform_pos = (ui->waveform_pos + 1) % 256;
        sample_count = 0;
    }
}
