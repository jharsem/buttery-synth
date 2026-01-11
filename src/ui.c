#include "ui.h"
#include "preset.h"
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
    CTRL_SUB_OSC_MIX,
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
    CTRL_VOLUME,
    CTRL_LFO_RATE,
    CTRL_LFO_DEPTH,
    CTRL_FILT_ENV_AMT,
    CTRL_PULSE_WIDTH,
    CTRL_PWM_RATE,
    CTRL_PWM_DEPTH
};

static const char *WAVE_NAMES[] = {"SIN", "SQR", "SAW", "TRI", "NSE"};
static const char *FILTER_NAMES[] = {"LP", "HP", "BP"};
static const char *LFO_NAMES[] = {"SIN", "TRI", "SAW", "SQR"};
static const char *PAGE_NAMES[] = {"OSC", "FLT", "FX", "MOD", "PRE", "SET"};
static const char *BUFFER_NAMES[] = {"512", "256", "128"};

void ui_init(UI *ui, Synth *synth, Effects *effects) {
    ui->synth = synth;
    ui->effects = effects;
    ui->current_page = 0;
    ui->selected_wave = synth->wave_type;
    ui->selected_wave2 = synth->wave_type2;
    ui->selected_filter = synth->filter_type;
    ui->selected_lfo = synth->lfo_type;
    ui->current_preset = 1;
    strcpy(ui->preset_name, "Init");
    ui->editing_name = false;
    ui->buffer_size = 1;  // Default to 256 (index 1)
    ui->panic_triggered = false;
    ui->buffer_changed = false;
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
    int tab_width = 55;
    int tab_height = 30;
    int tab_y = 5;
    for (int i = 0; i < 6; i++) {
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
        float new_sub = draw_slider("Sub", s->sub_osc_mix, 0.0f, 1.0f,
                                    panel_x + 10, panel_y + 90, CTRL_SUB_OSC_MIX, ui);
        if (new_sub != s->sub_osc_mix) {
            synth_set_sub_osc_mix(s, new_sub);
        }
        s->volume = draw_slider("Vol", s->volume, 0.0f, 1.0f,
                                panel_x + 10, panel_y + 120, CTRL_VOLUME, ui);

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

    } else if (ui->current_page == 2) {
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

    } else if (ui->current_page == 3) {
        // MOD PAGE: LFO + Filter Envelope
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH + 60, content_height, PANEL_COLOR);
        DrawText("LFO", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);
        int new_lfo = draw_button_row("Wave", LFO_NAMES, 4, ui->selected_lfo,
                                      panel_x + 10, panel_y + 25);
        if (new_lfo != ui->selected_lfo) {
            ui->selected_lfo = new_lfo;
            synth_set_lfo_type(s, (LFOWaveType)new_lfo);
        }
        float new_rate = draw_slider("Rate", s->lfo_rate, 0.1f, 20.0f,
                                     panel_x + 10, panel_y + 55, CTRL_LFO_RATE, ui);
        if (new_rate != s->lfo_rate) {
            synth_set_lfo_rate(s, new_rate);
        }
        float new_depth = draw_slider("Depth", s->lfo_depth, 0.0f, 1.0f,
                                      panel_x + 10, panel_y + 85, CTRL_LFO_DEPTH, ui);
        if (new_depth != s->lfo_depth) {
            synth_set_lfo_depth(s, new_depth);
        }

        panel_x += PANEL_WIDTH + 60 + PANEL_MARGIN;
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH + 60, content_height, PANEL_COLOR);
        DrawText("FILTER ENV", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);
        float new_amt = draw_slider("Amt", s->filter_env_amount, -1.0f, 1.0f,
                                    panel_x + 10, panel_y + 30, CTRL_FILT_ENV_AMT, ui);
        if (new_amt != s->filter_env_amount) {
            synth_set_filter_env_amount(s, new_amt);
        }
        DrawText("(Uses Amp ADSR)", panel_x + 10, panel_y + 60, 12, TEXT_COLOR);

        // PWM panel
        panel_x += PANEL_WIDTH + 60 + PANEL_MARGIN;
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH + 60, content_height, PANEL_COLOR);
        DrawText("PWM", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);
        float new_pw = draw_slider("Width", s->pulse_width, 0.05f, 0.95f,
                                   panel_x + 10, panel_y + 30, CTRL_PULSE_WIDTH, ui);
        if (new_pw != s->pulse_width) {
            synth_set_pulse_width(s, new_pw);
        }
        float new_pwm_rate = draw_slider("Rate", s->pwm_rate, 0.1f, 20.0f,
                                         panel_x + 10, panel_y + 60, CTRL_PWM_RATE, ui);
        if (new_pwm_rate != s->pwm_rate) {
            synth_set_pwm_rate(s, new_pwm_rate);
        }
        float new_pwm_depth = draw_slider("Depth", s->pwm_depth, 0.0f, 0.45f,
                                          panel_x + 10, panel_y + 90, CTRL_PWM_DEPTH, ui);
        if (new_pwm_depth != s->pwm_depth) {
            synth_set_pwm_depth(s, new_pwm_depth);
        }
        DrawText("(Square waves)", panel_x + 10, panel_y + 120, 12, TEXT_COLOR);

    } else if (ui->current_page == 4) {
        // PRESET PAGE
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH + 150, content_height, PANEL_COLOR);
        DrawText("PRESETS", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);

        // Preset number display with navigation
        char preset_str[16];
        snprintf(preset_str, sizeof(preset_str), "%03d", ui->current_preset);

        int nav_y = panel_y + 35;
        int btn_size = 36;

        // Previous button
        Rectangle prev_btn = {panel_x + 20, nav_y, btn_size, btn_size};
        DrawRectangleRec(prev_btn, SLIDER_BG);
        DrawText("<", prev_btn.x + 12, prev_btn.y + 8, 20, TEXT_COLOR);

        // Preset number
        DrawText(preset_str, panel_x + 70, nav_y + 6, 24, SLIDER_FG);

        // Next button
        Rectangle next_btn = {panel_x + 130, nav_y, btn_size, btn_size};
        DrawRectangleRec(next_btn, SLIDER_BG);
        DrawText(">", next_btn.x + 12, next_btn.y + 8, 20, TEXT_COLOR);

        // Check if preset exists and get name
        int exists = preset_exists(ui->current_preset);
        static int last_preset = -1;
        if (ui->current_preset != last_preset && !ui->editing_name) {
            last_preset = ui->current_preset;
            if (exists) {
                preset_get_name(ui->current_preset, ui->preset_name, sizeof(ui->preset_name));
            } else {
                snprintf(ui->preset_name, sizeof(ui->preset_name), "Preset %03d", ui->current_preset);
            }
        }

        // Display preset name (tappable for editing)
        Rectangle name_rect = {panel_x + 175, nav_y, 200, 30};
        DrawRectangleRec(name_rect, ui->editing_name ? SLIDER_BG : PANEL_COLOR);
        DrawRectangleLinesEx(name_rect, 1, ui->editing_name ? SLIDER_FG : SLIDER_BG);
        DrawText(ui->preset_name, panel_x + 180, nav_y + 6, 18,
                 ui->editing_name ? SLIDER_FG : (exists ? WAVE_COLOR : TEXT_COLOR));
        if (!ui->editing_name) {
            DrawText("[edit]", panel_x + 380, nav_y + 10, 12, TEXT_COLOR);
        }

        // Touch handling for navigation and name editing
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !ui->editing_name) {
            Vector2 mouse = GetTransformedTouch();
            if (CheckCollisionPointRec(mouse, prev_btn)) {
                ui->current_preset--;
                if (ui->current_preset < 1) ui->current_preset = MAX_PRESETS;
                last_preset = -1;
            }
            if (CheckCollisionPointRec(mouse, next_btn)) {
                ui->current_preset++;
                if (ui->current_preset > MAX_PRESETS) ui->current_preset = 1;
                last_preset = -1;
            }
            if (CheckCollisionPointRec(mouse, name_rect)) {
                ui->editing_name = true;
            }
        }

        // Load/Save buttons
        int action_y = panel_y + 85;
        Rectangle load_btn = {panel_x + 20, action_y, 80, 35};
        Rectangle save_btn = {panel_x + 110, action_y, 80, 35};

        Color load_color = exists ? SLIDER_FG : SLIDER_BG;

        DrawRectangleRec(load_btn, load_color);
        DrawText("LOAD", load_btn.x + 18, load_btn.y + 10, 14,
                 exists ? BG_COLOR : TEXT_COLOR);

        DrawRectangleRec(save_btn, SLIDER_FG);
        DrawText("SAVE", save_btn.x + 18, save_btn.y + 10, 14, BG_COLOR);

        // Touch handling for load/save
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !ui->editing_name) {
            Vector2 mouse = GetTransformedTouch();
            if (CheckCollisionPointRec(mouse, load_btn) && exists) {
                char path[64];
                preset_filename(ui->current_preset, path, sizeof(path));
                if (preset_load(path, ui->preset_name, sizeof(ui->preset_name), s, fx) == 0) {
                    ui->selected_wave = s->wave_type;
                    ui->selected_wave2 = s->wave_type2;
                    ui->selected_filter = s->filter_type;
                    ui->selected_lfo = s->lfo_type;
                }
            }
            if (CheckCollisionPointRec(mouse, save_btn)) {
                char path[64];
                preset_filename(ui->current_preset, path, sizeof(path));
                if (ui->preset_name[0] == '\0') {
                    snprintf(ui->preset_name, sizeof(ui->preset_name), "Preset %03d", ui->current_preset);
                }
                preset_save(path, ui->preset_name, s, fx);
                last_preset = -1;
            }
        }

        // Status text
        if (!ui->editing_name) {
            if (exists) {
                DrawText("Tap LOAD to recall", panel_x + 20, panel_y + 135, 12, TEXT_COLOR);
            } else {
                DrawText("Empty - tap SAVE", panel_x + 20, panel_y + 135, 12, TEXT_COLOR);
            }
        }

        // On-screen keyboard when editing name
        if (ui->editing_name) {
            int kb_x = panel_x + PANEL_WIDTH + 180;
            int kb_y = panel_y + 10;
            int key_w = 32;
            int key_h = 28;
            int key_gap = 3;

            DrawRectangle(kb_x - 10, kb_y - 5, 380, 175, (Color){35, 35, 50, 255});
            DrawText("EDIT NAME", kb_x, kb_y, 14, SLIDER_FG);
            kb_y += 20;

            // Keyboard rows
            const char *rows[] = {
                "QWERTYUIOP",
                "ASDFGHJKL",
                "ZXCVBNM",
                "0123456789"
            };
            int row_offsets[] = {0, 15, 30, 0};

            for (int row = 0; row < 4; row++) {
                int rx = kb_x + row_offsets[row];
                for (int col = 0; rows[row][col]; col++) {
                    Rectangle key = {rx + col * (key_w + key_gap), kb_y + row * (key_h + key_gap), key_w, key_h};
                    DrawRectangleRec(key, SLIDER_BG);
                    char ch[2] = {rows[row][col], 0};
                    DrawText(ch, key.x + 11, key.y + 6, 16, TEXT_COLOR);

                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        Vector2 mouse = GetTransformedTouch();
                        if (CheckCollisionPointRec(mouse, key)) {
                            int len = strlen(ui->preset_name);
                            if (len < 20) {
                                ui->preset_name[len] = rows[row][col];
                                ui->preset_name[len + 1] = '\0';
                            }
                        }
                    }
                }
            }

            // Space, Backspace, Done buttons
            int btn_y = kb_y + 4 * (key_h + key_gap);

            Rectangle space_btn = {kb_x, btn_y, 100, key_h};
            DrawRectangleRec(space_btn, SLIDER_BG);
            DrawText("SPACE", space_btn.x + 25, space_btn.y + 6, 14, TEXT_COLOR);

            Rectangle back_btn = {kb_x + 110, btn_y, 80, key_h};
            DrawRectangleRec(back_btn, SLIDER_BG);
            DrawText("<DEL", back_btn.x + 18, back_btn.y + 6, 14, TEXT_COLOR);

            Rectangle done_btn = {kb_x + 200, btn_y, 80, key_h};
            DrawRectangleRec(done_btn, SLIDER_FG);
            DrawText("DONE", done_btn.x + 18, done_btn.y + 6, 14, BG_COLOR);

            Rectangle clear_btn = {kb_x + 290, btn_y, 70, key_h};
            DrawRectangleRec(clear_btn, SLIDER_BG);
            DrawText("CLR", clear_btn.x + 18, clear_btn.y + 6, 14, TEXT_COLOR);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mouse = GetTransformedTouch();
                if (CheckCollisionPointRec(mouse, space_btn)) {
                    int len = strlen(ui->preset_name);
                    if (len < 20) {
                        ui->preset_name[len] = ' ';
                        ui->preset_name[len + 1] = '\0';
                    }
                }
                if (CheckCollisionPointRec(mouse, back_btn)) {
                    int len = strlen(ui->preset_name);
                    if (len > 0) {
                        ui->preset_name[len - 1] = '\0';
                    }
                }
                if (CheckCollisionPointRec(mouse, done_btn)) {
                    ui->editing_name = false;
                }
                if (CheckCollisionPointRec(mouse, clear_btn)) {
                    ui->preset_name[0] = '\0';
                }
            }
        }

    } else {
        // SETTINGS PAGE
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH + 100, content_height, PANEL_COLOR);
        DrawText("SETTINGS", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);

        // Buffer size selector
        DrawText("Audio Buffer:", panel_x + 20, panel_y + 40, 14, TEXT_COLOR);

        int buf_x = panel_x + 130;
        int buf_y = panel_y + 35;
        int buf_w = 60;
        int buf_h = 28;

        for (int i = 0; i < 3; i++) {
            Rectangle buf_btn = {buf_x + i * (buf_w + 5), buf_y, buf_w, buf_h};
            Color btn_col = (ui->buffer_size == i) ? SLIDER_FG : SLIDER_BG;
            DrawRectangleRec(buf_btn, btn_col);
            int tw = MeasureText(BUFFER_NAMES[i], 14);
            DrawText(BUFFER_NAMES[i], buf_btn.x + (buf_w - tw) / 2, buf_btn.y + 7, 14,
                     (ui->buffer_size == i) ? BG_COLOR : TEXT_COLOR);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mouse = GetTransformedTouch();
                if (CheckCollisionPointRec(mouse, buf_btn) && ui->buffer_size != i) {
                    ui->buffer_size = i;
                    ui->buffer_changed = true;
                }
            }
        }

        // Show latency info
        const char *latency_info[] = {"~11.6ms", "~5.8ms", "~2.9ms"};
        DrawText(latency_info[ui->buffer_size], buf_x + 200, buf_y + 7, 14, WAVE_COLOR);

        // Buffer changes apply immediately at runtime

        // Panic button
        panel_x += PANEL_WIDTH + 100 + PANEL_MARGIN;
        DrawRectangle(panel_x, panel_y, PANEL_WIDTH, content_height, PANEL_COLOR);
        DrawText("MIDI", panel_x + 10, panel_y + 5, 16, TEXT_COLOR);

        Rectangle panic_btn = {panel_x + 20, panel_y + 40, 120, 50};
        DrawRectangleRec(panic_btn, (Color){200, 60, 60, 255});
        DrawText("PANIC", panic_btn.x + 30, panic_btn.y + 16, 18, WHITE);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetTransformedTouch();
            if (CheckCollisionPointRec(mouse, panic_btn)) {
                ui->panic_triggered = true;
            }
        }

        DrawText("All notes off", panel_x + 20, panel_y + 100, 12, TEXT_COLOR);
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
