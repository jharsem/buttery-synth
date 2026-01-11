#include "preset.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

void preset_filename(int slot, char *buffer, int buffer_size) {
    snprintf(buffer, buffer_size, "%s/%03d.json", PRESET_DIR, slot);
}

int preset_exists(int slot) {
    char path[64];
    preset_filename(slot, path, sizeof(path));
    struct stat st;
    return (stat(path, &st) == 0);
}

// Helper to write a JSON string, escaping quotes
static void write_json_string(FILE *f, const char *key, const char *value) {
    fprintf(f, "  \"%s\": \"", key);
    for (const char *p = value; *p; p++) {
        if (*p == '"' || *p == '\\') {
            fputc('\\', f);
        }
        fputc(*p, f);
    }
    fprintf(f, "\"");
}

int preset_save(const char *filepath, const char *name, Synth *s, Effects *fx, Arpeggiator *arp) {
    // Ensure presets directory exists
    mkdir(PRESET_DIR, 0755);

    FILE *f = fopen(filepath, "w");
    if (!f) return -1;

    fprintf(f, "{\n");

    // Preset name
    write_json_string(f, "name", name ? name : "Untitled");
    fprintf(f, ",\n");

    // Oscillator section
    fprintf(f, "  \"oscillator\": {\n");
    fprintf(f, "    \"wave1\": %d,\n", s->wave_type);
    fprintf(f, "    \"wave2\": %d,\n", s->wave_type2);
    fprintf(f, "    \"mix\": %.4f,\n", s->osc_mix);
    fprintf(f, "    \"detune\": %.4f,\n", s->osc2_detune);
    fprintf(f, "    \"sub_mix\": %.4f,\n", s->sub_osc_mix);
    fprintf(f, "    \"pulse_width\": %.4f,\n", s->pulse_width);
    fprintf(f, "    \"pwm_rate\": %.4f,\n", s->pwm_rate);
    fprintf(f, "    \"pwm_depth\": %.4f,\n", s->pwm_depth);
    fprintf(f, "    \"unison_count\": %d,\n", s->unison_count);
    fprintf(f, "    \"unison_spread\": %.4f,\n", s->unison_spread);
    fprintf(f, "    \"wavetable_type\": %d,\n", s->wavetable_type);
    fprintf(f, "    \"wt_position\": %.4f\n", s->wt_position);
    fprintf(f, "  },\n");

    // Arpeggiator section
    fprintf(f, "  \"arpeggiator\": {\n");
    fprintf(f, "    \"enabled\": %d,\n", arp->enabled);
    fprintf(f, "    \"pattern\": %d,\n", arp->pattern);
    fprintf(f, "    \"division\": %d,\n", arp->division);
    fprintf(f, "    \"tempo\": %.4f,\n", arp->tempo);
    fprintf(f, "    \"octaves\": %d,\n", arp->octaves);
    fprintf(f, "    \"gate\": %.4f\n", arp->gate);
    fprintf(f, "  },\n");

    // Filter section
    fprintf(f, "  \"filter\": {\n");
    fprintf(f, "    \"type\": %d,\n", s->filter_type);
    fprintf(f, "    \"cutoff\": %.4f,\n", s->filter_cutoff);
    fprintf(f, "    \"resonance\": %.4f\n", s->filter_resonance);
    fprintf(f, "  },\n");

    // Amplitude envelope
    fprintf(f, "  \"amp_env\": {\n");
    fprintf(f, "    \"attack\": %.4f,\n", s->attack);
    fprintf(f, "    \"decay\": %.4f,\n", s->decay);
    fprintf(f, "    \"sustain\": %.4f,\n", s->sustain);
    fprintf(f, "    \"release\": %.4f\n", s->release);
    fprintf(f, "  },\n");

    // Filter envelope
    fprintf(f, "  \"filter_env\": {\n");
    fprintf(f, "    \"attack\": %.4f,\n", s->filter_env_attack);
    fprintf(f, "    \"decay\": %.4f,\n", s->filter_env_decay);
    fprintf(f, "    \"sustain\": %.4f,\n", s->filter_env_sustain);
    fprintf(f, "    \"release\": %.4f,\n", s->filter_env_release);
    fprintf(f, "    \"amount\": %.4f\n", s->filter_env_amount);
    fprintf(f, "  },\n");

    // LFO
    fprintf(f, "  \"lfo\": {\n");
    fprintf(f, "    \"type\": %d,\n", s->lfo_type);
    fprintf(f, "    \"rate\": %.4f,\n", s->lfo_rate);
    fprintf(f, "    \"depth\": %.4f\n", s->lfo_depth);
    fprintf(f, "  },\n");

    // Effects
    fprintf(f, "  \"effects\": {\n");
    fprintf(f, "    \"delay_time\": %.4f,\n", fx->delay.time);
    fprintf(f, "    \"delay_feedback\": %.4f,\n", fx->delay.feedback);
    fprintf(f, "    \"delay_mix\": %.4f,\n", fx->delay.mix);
    fprintf(f, "    \"reverb_mix\": %.4f,\n", fx->reverb.mix);
    fprintf(f, "    \"reverb_size\": %.4f,\n", fx->reverb.roomsize);
    fprintf(f, "    \"dist_drive\": %.4f,\n", fx->distortion.drive);
    fprintf(f, "    \"dist_mix\": %.4f\n", fx->distortion.mix);
    fprintf(f, "  },\n");

    // Master
    fprintf(f, "  \"volume\": %.4f\n", s->volume);

    fprintf(f, "}\n");

    fclose(f);
    return 0;
}

// Simple JSON parser helpers
static int skip_whitespace(FILE *f) {
    int c;
    while ((c = fgetc(f)) != EOF && isspace(c));
    return c;
}

static int read_string(FILE *f, char *buf, int buf_size) {
    int i = 0;
    int c;
    while ((c = fgetc(f)) != EOF && c != '"' && i < buf_size - 1) {
        if (c == '\\') {
            c = fgetc(f);
            if (c == EOF) break;
        }
        buf[i++] = c;
    }
    buf[i] = '\0';
    return (c == '"') ? 0 : -1;
}

static float read_number(FILE *f) {
    float val = 0;
    fscanf(f, "%f", &val);
    return val;
}

static int read_int(FILE *f) {
    int val = 0;
    fscanf(f, "%d", &val);
    return val;
}

int preset_load(const char *filepath, char *name, int name_size, Synth *s, Effects *fx, Arpeggiator *arp) {
    FILE *f = fopen(filepath, "r");
    if (!f) return -1;

    char key[64];
    char section[64] = "";
    int c;

    if (name && name_size > 0) name[0] = '\0';

    while ((c = skip_whitespace(f)) != EOF) {
        if (c == '{' || c == ',' || c == '}') continue;

        if (c == '"') {
            // Read key
            if (read_string(f, key, sizeof(key)) < 0) break;

            // Skip to colon
            while ((c = fgetc(f)) != EOF && c != ':');
            if (c == EOF) break;

            // Get value
            c = skip_whitespace(f);

            if (c == '{') {
                // Nested object - set section name
                strncpy(section, key, sizeof(section) - 1);
                continue;
            } else if (c == '"') {
                // String value
                char strval[64];
                read_string(f, strval, sizeof(strval));
                if (strcmp(key, "name") == 0 && name) {
                    strncpy(name, strval, name_size - 1);
                    name[name_size - 1] = '\0';
                }
            } else {
                // Number value
                ungetc(c, f);
                float val = read_number(f);

                // Apply based on section and key
                if (strcmp(section, "oscillator") == 0) {
                    if (strcmp(key, "wave1") == 0) synth_set_wave_type(s, (WaveType)(int)val);
                    else if (strcmp(key, "wave2") == 0) synth_set_wave_type2(s, (WaveType)(int)val);
                    else if (strcmp(key, "mix") == 0) synth_set_osc_mix(s, val);
                    else if (strcmp(key, "detune") == 0) synth_set_osc2_detune(s, val);
                    else if (strcmp(key, "sub_mix") == 0) synth_set_sub_osc_mix(s, val);
                    else if (strcmp(key, "pulse_width") == 0) synth_set_pulse_width(s, val);
                    else if (strcmp(key, "pwm_rate") == 0) synth_set_pwm_rate(s, val);
                    else if (strcmp(key, "pwm_depth") == 0) synth_set_pwm_depth(s, val);
                    else if (strcmp(key, "unison_count") == 0) synth_set_unison_count(s, (int)val);
                    else if (strcmp(key, "unison_spread") == 0) synth_set_unison_spread(s, val);
                    else if (strcmp(key, "wavetable_type") == 0) synth_set_wavetable(s, (WavetableType)(int)val);
                    else if (strcmp(key, "wt_position") == 0) synth_set_wt_position(s, val);
                } else if (strcmp(section, "filter") == 0) {
                    if (strcmp(key, "type") == 0) s->filter_type = (FilterType)(int)val;
                    else if (strcmp(key, "cutoff") == 0) s->filter_cutoff = val;
                    else if (strcmp(key, "resonance") == 0) s->filter_resonance = val;
                    synth_set_filter(s, s->filter_cutoff, s->filter_resonance, s->filter_type);
                } else if (strcmp(section, "amp_env") == 0) {
                    if (strcmp(key, "attack") == 0) s->attack = val;
                    else if (strcmp(key, "decay") == 0) s->decay = val;
                    else if (strcmp(key, "sustain") == 0) s->sustain = val;
                    else if (strcmp(key, "release") == 0) s->release = val;
                    synth_set_adsr(s, s->attack, s->decay, s->sustain, s->release);
                } else if (strcmp(section, "filter_env") == 0) {
                    if (strcmp(key, "attack") == 0) s->filter_env_attack = val;
                    else if (strcmp(key, "decay") == 0) s->filter_env_decay = val;
                    else if (strcmp(key, "sustain") == 0) s->filter_env_sustain = val;
                    else if (strcmp(key, "release") == 0) s->filter_env_release = val;
                    else if (strcmp(key, "amount") == 0) synth_set_filter_env_amount(s, val);
                    synth_set_filter_env_adsr(s, s->filter_env_attack, s->filter_env_decay,
                                              s->filter_env_sustain, s->filter_env_release);
                } else if (strcmp(section, "lfo") == 0) {
                    if (strcmp(key, "type") == 0) synth_set_lfo_type(s, (LFOWaveType)(int)val);
                    else if (strcmp(key, "rate") == 0) synth_set_lfo_rate(s, val);
                    else if (strcmp(key, "depth") == 0) synth_set_lfo_depth(s, val);
                } else if (strcmp(section, "arpeggiator") == 0) {
                    if (strcmp(key, "enabled") == 0) arp->enabled = (int)val;
                    else if (strcmp(key, "pattern") == 0) arp->pattern = (ArpPattern)(int)val;
                    else if (strcmp(key, "division") == 0) arp->division = (ArpDivision)(int)val;
                    else if (strcmp(key, "tempo") == 0) arp->tempo = val;
                    else if (strcmp(key, "octaves") == 0) arp->octaves = (int)val;
                    else if (strcmp(key, "gate") == 0) arp->gate = val;
                } else if (strcmp(section, "effects") == 0) {
                    if (strcmp(key, "delay_time") == 0) fx->delay.time = val;
                    else if (strcmp(key, "delay_feedback") == 0) fx->delay.feedback = val;
                    else if (strcmp(key, "delay_mix") == 0) fx->delay.mix = val;
                    else if (strcmp(key, "reverb_mix") == 0) fx->reverb.mix = val;
                    else if (strcmp(key, "reverb_size") == 0) fx->reverb.roomsize = val;
                    else if (strcmp(key, "dist_drive") == 0) fx->distortion.drive = val;
                    else if (strcmp(key, "dist_mix") == 0) fx->distortion.mix = val;
                } else if (strcmp(key, "volume") == 0) {
                    synth_set_volume(s, val);
                }
            }
        }
    }

    fclose(f);
    return 0;
}

int preset_get_name(int slot, char *name, int name_size) {
    char path[64];
    preset_filename(slot, path, sizeof(path));

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char key[64];
    int c;

    name[0] = '\0';

    // Simple parse just to find "name" field
    while ((c = skip_whitespace(f)) != EOF) {
        if (c == '"') {
            if (read_string(f, key, sizeof(key)) < 0) break;

            while ((c = fgetc(f)) != EOF && c != ':');
            if (c == EOF) break;

            c = skip_whitespace(f);

            if (strcmp(key, "name") == 0 && c == '"') {
                read_string(f, name, name_size);
                fclose(f);
                return 0;
            }
        }
    }

    fclose(f);
    return -1;
}
